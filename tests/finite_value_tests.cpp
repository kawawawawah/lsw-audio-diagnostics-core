// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
    static_assert(!std::is_copy_constructible_v<lsw::audio_diag::Analyzer<float>>,
                  "Analyzer instances must not be copyable.");
    static_assert(!std::is_copy_assignable_v<lsw::audio_diag::Analyzer<float>>,
                  "Analyzer instances must not be copyable.");

    [[nodiscard]] lsw::audio_diag::Snapshot processFiniteMono(const std::vector<float>& input)
    {
        lsw::audio_diag::Analyzer<float> analyzer;
        auto config = lsw::audio_diag::test::quickConfig(1U, input.size());
        if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
        {
            return {};
        }
        const float* channels[] { input.data() };
        analyzer.process(channels, 1U, input.size());
        return analyzer.getSnapshot();
    }

    [[nodiscard]] bool metricsAreFinite(const lsw::audio_diag::ChannelMetrics& metrics)
    {
        return std::isfinite(metrics.samplePeak) && std::isfinite(metrics.smoothedRms)
               && std::isfinite(metrics.rmsDbfs) && std::isfinite(metrics.dcOffset)
               && std::isfinite(metrics.maximumAbsoluteSample);
    }
}

LSW_TEST_CASE(nan_samples_are_counted)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::nanSignal<float>(32U));
    LSW_CHECK_EQ(snapshot.channels[0].nanCount, 1U);
    LSW_CHECK_EQ(snapshot.channels[0].invalidSampleCount, 1U);
}

LSW_TEST_CASE(positive_infinity_samples_are_counted)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::positiveInfinitySignal<float>(32U));
    LSW_CHECK_EQ(snapshot.channels[0].positiveInfinityCount, 1U);
    LSW_CHECK_EQ(snapshot.channels[0].invalidSampleCount, 1U);
}

LSW_TEST_CASE(negative_infinity_samples_are_counted)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::negativeInfinitySignal<float>(32U));
    LSW_CHECK_EQ(snapshot.channels[0].negativeInfinityCount, 1U);
    LSW_CHECK_EQ(snapshot.channels[0].invalidSampleCount, 1U);
}

LSW_TEST_CASE(denormal_samples_are_counted_and_sanitized)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::denormalSignal<float>(32U));
    LSW_CHECK_EQ(snapshot.channels[0].denormalCount, 32U);
    LSW_CHECK_NEAR(snapshot.channels[0].maximumAbsoluteSample, 0.0, 1.0e-12);
}

LSW_TEST_CASE(mixed_valid_and_invalid_samples_keep_valid_measurements)
{
    std::vector<float> input = lsw::audio_diag::test::constant<float>(32U, 0.5);
    input[3] = std::numeric_limits<float>::quiet_NaN();
    input[11] = std::numeric_limits<float>::infinity();
    const auto snapshot = processFiniteMono(input);
    LSW_CHECK_EQ(snapshot.channels[0].invalidSampleCount, 2U);
    LSW_CHECK(snapshot.channels[0].maximumAbsoluteSample >= 0.5);
}

LSW_TEST_CASE(invalid_samples_never_propagate_to_snapshot_values)
{
    std::vector<float> input = lsw::audio_diag::test::nanSignal<float>(32U);
    input[8] = std::numeric_limits<float>::infinity();
    input[16] = -std::numeric_limits<float>::infinity();
    const auto snapshot = processFiniteMono(input);
    LSW_CHECK(metricsAreFinite(snapshot.channels[0]));
}

LSW_TEST_CASE(invalid_stereo_samples_do_not_break_correlation)
{
    auto left = lsw::audio_diag::test::constant<float>(128U, 0.5);
    auto right = lsw::audio_diag::test::constant<float>(128U, 0.5);
    left[0] = std::numeric_limits<float>::quiet_NaN();
    right[1] = std::numeric_limits<float>::infinity();
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(2U, 128U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const float* channels[] { left.data(), right.data() };
    analyzer.process(channels, 2U, left.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(std::isfinite(snapshot.stereo.correlation));
    LSW_CHECK(std::isfinite(snapshot.stereo.channelBalanceDb));
    LSW_CHECK(std::isfinite(snapshot.stereo.monoCompatibilityScore));
    LSW_CHECK(snapshot.stereo.correlation >= -1.0 && snapshot.stereo.correlation <= 1.0);
    LSW_CHECK(snapshot.stereo.monoCompatibilityScore >= 0.0
              && snapshot.stereo.monoCompatibilityScore <= 1.0);
}

LSW_TEST_CASE(float_processing_is_supported)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::constant<float>(32U, 0.25));
    LSW_CHECK_NEAR(snapshot.channels[0].samplePeak, 0.25, 1.0e-6);
}

LSW_TEST_CASE(double_processing_is_supported)
{
    lsw::audio_diag::Analyzer<double> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 32U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto input = lsw::audio_diag::test::constant<double>(32U, 0.25);
    const double* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    LSW_CHECK_NEAR(analyzer.getSnapshot().channels[0].samplePeak, 0.25, 1.0e-12);
}

LSW_TEST_CASE(mono_configuration_reports_one_active_channel)
{
    const auto snapshot = processFiniteMono(lsw::audio_diag::test::constant<float>(32U, 0.25));
    LSW_CHECK_EQ(snapshot.activeChannelCount, 1U);
}

LSW_TEST_CASE(stereo_configuration_reports_two_active_channels)
{
    const auto signal = lsw::audio_diag::test::constant<float>(32U, 0.25);
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(2U, 32U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const float* channels[] { signal.data(), signal.data() };
    analyzer.process(channels, 2U, signal.size());
    LSW_CHECK_EQ(analyzer.getSnapshot().activeChannelCount, 2U);
}

LSW_TEST_CASE(repeated_snapshot_reads_are_stable)
{
    const auto input = lsw::audio_diag::test::constant<float>(32U, 0.25);
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 32U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    const auto first = analyzer.getSnapshot();
    const auto second = analyzer.getSnapshot();
    LSW_CHECK_EQ(first.processedSampleCount, second.processedSampleCount);
    LSW_CHECK_NEAR(first.channels[0].samplePeak, second.channels[0].samplePeak, 1.0e-12);
}

LSW_TEST_CASE(snapshot_contains_consistent_prepared_state)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(2U, 32U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(snapshot.prepared);
    LSW_CHECK_EQ(snapshot.activeChannelCount, 2U);
    LSW_CHECK_NEAR(snapshot.sampleRate, config.sampleRate, 1.0e-12);
}

LSW_TEST_CASE(all_invalid_classifications_are_independently_counted)
{
    std::vector<float> input = lsw::audio_diag::test::constant<float>(8U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    input[1] = std::numeric_limits<float>::infinity();
    input[2] = -std::numeric_limits<float>::infinity();
    input[3] = std::numeric_limits<float>::denorm_min();
    const auto snapshot = processFiniteMono(input);
    LSW_CHECK_EQ(snapshot.channels[0].invalidSampleCount, 3U);
    LSW_CHECK_EQ(snapshot.channels[0].denormalCount, 1U);
}

LSW_TEST_CASE(analyzer_move_preserves_published_metrics)
{
    lsw::audio_diag::Analyzer<float> original;
    auto config = lsw::audio_diag::test::quickConfig(1U, 32U);
    LSW_CHECK(lsw::audio_diag::isSuccess(original.prepare(config)));
    const auto input = lsw::audio_diag::test::constant<float>(32U, 0.75);
    const float* channels[] { input.data() };
    original.process(channels, 1U, input.size());
    lsw::audio_diag::Analyzer<float> moved(std::move(original));
    LSW_CHECK_NEAR(moved.getSnapshot().channels[0].maximumAbsoluteSample, 0.75, 1.0e-6);
}

LSW_TEST_CASE(concurrent_snapshot_reads_are_safe_during_processing)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(2U, 64U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto left = lsw::audio_diag::test::sine<float>(64U);
    const auto right = lsw::audio_diag::test::sine<float>(64U, 0.5);
    const float* channels[] { left.data(), right.data() };
    std::atomic<bool> stop { false };
    std::atomic<bool> valid { true };
    std::thread reader([&analyzer, &stop, &valid]() {
        while (!stop.load(std::memory_order_acquire))
        {
            const auto snapshot = analyzer.getSnapshot();
            if (!metricsAreFinite(snapshot.channels[0]) || !metricsAreFinite(snapshot.channels[1])
                || !std::isfinite(snapshot.stereo.correlation))
            {
                valid.store(false, std::memory_order_release);
                break;
            }
        }
    });
    for (std::size_t iteration = 0U; iteration < 1000U; ++iteration)
    {
        analyzer.process(channels, 2U, left.size());
    }
    stop.store(true, std::memory_order_release);
    reader.join();
    LSW_CHECK(valid.load(std::memory_order_acquire));
    LSW_CHECK_EQ(analyzer.getSnapshot().processedBlockCount, 1000U);
}
