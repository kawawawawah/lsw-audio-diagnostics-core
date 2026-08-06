// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "lsw/audio_diag/detail/correlation_tracker.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace
{
    [[nodiscard]] lsw::audio_diag::Snapshot processStereo(const std::vector<float>& left,
                                                           const std::vector<float>& right,
                                                           lsw::audio_diag::AnalyzerConfig config)
    {
        config.numberOfChannels = 2U;
        config.maximumBlockSize = std::max(left.size(), right.size());
        lsw::audio_diag::Analyzer<float> analyzer;
        if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
        {
            return {};
        }
        const float* channels[] { left.data(), right.data() };
        analyzer.process(channels, 2U, std::min(left.size(), right.size()));
        return analyzer.getSnapshot();
    }

    [[nodiscard]] lsw::audio_diag::AnalyzerConfig stereoConfig(const std::size_t blockSize = 4096U)
    {
        return lsw::audio_diag::test::quickConfig(2U, blockSize);
    }
}

LSW_TEST_CASE(identical_stereo_is_detected)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.7);
    const auto snapshot = processStereo(left, left, stereoConfig());
    LSW_CHECK(snapshot.stereo.identicalChannels);
}

LSW_TEST_CASE(opposite_polarity_stereo_is_detected)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.7);
    const auto right = lsw::audio_diag::test::constant<float>(256U, -0.7);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK(snapshot.stereo.reversedPolarity);
}

LSW_TEST_CASE(identical_signals_have_positive_one_correlation)
{
    const auto left = lsw::audio_diag::test::sine<float>(480U);
    const auto snapshot = processStereo(left, left, stereoConfig());
    LSW_CHECK_NEAR(snapshot.stereo.correlation, 1.0, 1.0e-6);
}

LSW_TEST_CASE(opposite_signals_have_negative_one_correlation)
{
    const auto left = lsw::audio_diag::test::sine<float>(480U);
    std::vector<float> right = left;
    for (float& value : right)
    {
        value = -value;
    }
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK_NEAR(snapshot.stereo.correlation, -1.0, 1.0e-6);
}

LSW_TEST_CASE(deterministic_uncorrelated_noise_has_neutral_correlation)
{
    std::vector<float> left = lsw::audio_diag::test::deterministicNoise<float>(48000U);
    std::vector<float> right = left;
    std::rotate(right.begin(), right.begin() + 997, right.end());
    auto config = stereoConfig(48000U);
    config.correlationTimeConstantSeconds = 0.02;
    const auto snapshot = processStereo(left, right, config);
    LSW_CHECK_NEAR(snapshot.stereo.correlation, 0.0, 0.15);
}

LSW_TEST_CASE(left_only_state_is_detected)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.8);
    const auto right = lsw::audio_diag::test::silence<float>(256U);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK(snapshot.stereo.leftOnly);
    LSW_CHECK(!snapshot.stereo.rightOnly);
    LSW_CHECK(!snapshot.stereo.reversedPolarity);
}

LSW_TEST_CASE(right_only_state_is_detected)
{
    const auto left = lsw::audio_diag::test::silence<float>(256U);
    const auto right = lsw::audio_diag::test::constant<float>(256U, 0.8);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK(snapshot.stereo.rightOnly);
    LSW_CHECK(!snapshot.stereo.leftOnly);
}

LSW_TEST_CASE(balanced_stereo_has_zero_balance)
{
    const auto signal = lsw::audio_diag::test::constant<float>(256U, 0.5);
    const auto snapshot = processStereo(signal, signal, stereoConfig());
    LSW_CHECK_NEAR(snapshot.stereo.channelBalanceDb, 0.0, 1.0e-6);
}

LSW_TEST_CASE(left_dominant_stereo_has_positive_balance)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.8);
    const auto right = lsw::audio_diag::test::constant<float>(256U, 0.4);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK(snapshot.stereo.channelBalanceDb > 5.0);
    LSW_CHECK(snapshot.stereo.channelBalanceDb < 7.1);
}

LSW_TEST_CASE(right_dominant_stereo_has_negative_balance)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.4);
    const auto right = lsw::audio_diag::test::constant<float>(256U, 0.8);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK(snapshot.stereo.channelBalanceDb < -5.0);
    LSW_CHECK(snapshot.stereo.channelBalanceDb > -7.1);
}

LSW_TEST_CASE(opposite_polarity_has_zero_mono_compatibility_score)
{
    const auto left = lsw::audio_diag::test::constant<float>(256U, 0.5);
    const auto right = lsw::audio_diag::test::constant<float>(256U, -0.5);
    const auto snapshot = processStereo(left, right, stereoConfig());
    LSW_CHECK_NEAR(snapshot.stereo.monoCompatibilityScore, 0.0, 1.0e-6);
}

LSW_TEST_CASE(neutral_correlation_has_half_mono_compatibility_score)
{
    std::vector<float> left = lsw::audio_diag::test::deterministicNoise<float>(48000U);
    std::vector<float> right = left;
    std::rotate(right.begin(), right.begin() + 997, right.end());
    auto config = stereoConfig(48000U);
    config.correlationTimeConstantSeconds = 0.02;
    const auto snapshot = processStereo(left, right, config);
    LSW_CHECK_NEAR(snapshot.stereo.monoCompatibilityScore, 0.5, 0.075);
}

LSW_TEST_CASE(identical_stereo_has_one_mono_compatibility_score)
{
    const auto signal = lsw::audio_diag::test::constant<float>(256U, 0.5);
    const auto snapshot = processStereo(signal, signal, stereoConfig());
    LSW_CHECK_NEAR(snapshot.stereo.monoCompatibilityScore, 1.0, 1.0e-6);
}

LSW_TEST_CASE(silent_stereo_has_neutral_finite_correlation_without_reverse_polarity)
{
    const auto silence = lsw::audio_diag::test::silence<float>(256U);
    const auto snapshot = processStereo(silence, silence, stereoConfig());
    LSW_CHECK(std::isfinite(snapshot.stereo.correlation));
    LSW_CHECK_NEAR(snapshot.stereo.correlation, 0.0, 1.0e-12);
    LSW_CHECK(std::isfinite(snapshot.stereo.channelBalanceDb));
    LSW_CHECK(snapshot.stereo.monoCompatibilityScore >= 0.0);
    LSW_CHECK(snapshot.stereo.monoCompatibilityScore <= 1.0);
    LSW_CHECK(!snapshot.stereo.reversedPolarity);
}

LSW_TEST_CASE(finite_difference_of_equal_positive_double_max_is_zero)
{
    lsw::audio_diag::detail::CorrelationTracker tracker;
    tracker.configure(stereoConfig(1U));
    tracker.beginBlock();
    const double maximum = std::numeric_limits<double>::max();
    tracker.processPair(maximum, maximum);
    LSW_CHECK_NEAR(tracker.maximumAbsoluteDifference(), 0.0, 0.0);
}

LSW_TEST_CASE(finite_difference_of_equal_negative_double_max_is_zero)
{
    lsw::audio_diag::detail::CorrelationTracker tracker;
    tracker.configure(stereoConfig(1U));
    tracker.beginBlock();
    const double maximum = std::numeric_limits<double>::max();
    tracker.processPair(-maximum, -maximum);
    LSW_CHECK_NEAR(tracker.maximumAbsoluteDifference(), 0.0, 0.0);
}

LSW_TEST_CASE(finite_difference_of_opposite_double_max_saturates_finitely)
{
    lsw::audio_diag::detail::CorrelationTracker tracker;
    tracker.configure(stereoConfig(1U));
    tracker.beginBlock();
    const double maximum = std::numeric_limits<double>::max();
    tracker.processPair(maximum, -maximum);
    LSW_CHECK(std::isfinite(tracker.maximumAbsoluteDifference()));
    LSW_CHECK_EQ(tracker.maximumAbsoluteDifference(), maximum);
}

LSW_TEST_CASE(identical_extreme_double_channels_have_zero_difference_and_finite_snapshot)
{
    auto config = stereoConfig(1U);
    lsw::audio_diag::Analyzer<double> analyzer;
    LSW_CHECK_EQ(analyzer.prepare(config), lsw::audio_diag::PrepareResult::success);
    const double maximum = std::numeric_limits<double>::max();
    const double left[] { maximum };
    const double right[] { maximum };
    const double* channels[] { left, right };
    analyzer.process(channels, 2U, 1U);
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(snapshot.stereo.identicalChannels);
    LSW_CHECK(std::isfinite(snapshot.channels[0].smoothedRms));
    LSW_CHECK(std::isfinite(snapshot.channels[0].rmsDbfs));
    LSW_CHECK(std::isfinite(snapshot.stereo.correlation));
    LSW_CHECK(std::isfinite(snapshot.stereo.channelBalanceDb));
}
