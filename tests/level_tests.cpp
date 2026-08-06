// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <cstddef>

namespace
{
    [[nodiscard]] lsw::audio_diag::Snapshot processMono(const std::vector<float>& input,
                                                         lsw::audio_diag::AnalyzerConfig config)
    {
        config.numberOfChannels = 1U;
        config.maximumBlockSize = input.empty() ? 1U : input.size();
        lsw::audio_diag::Analyzer<float> analyzer;
        if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
        {
            return {};
        }
        const float* channels[] { input.data() };
        analyzer.process(channels, 1U, input.size());
        return analyzer.getSnapshot();
    }

    void checkBlockSize(const std::size_t size)
    {
        auto config = lsw::audio_diag::test::quickConfig(1U, size);
        const auto input = lsw::audio_diag::test::constant<float>(size, 0.25);
        const auto snapshot = processMono(input, config);
        LSW_CHECK_EQ(snapshot.processedSampleCount, static_cast<std::uint64_t>(size));
        LSW_CHECK_NEAR(snapshot.channels[0].samplePeak, 0.25, 1.0e-6);
    }
}

LSW_TEST_CASE(silence_reaches_configured_silence_state)
{
    const auto snapshot = processMono(lsw::audio_diag::test::silence<float>(128U),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK(snapshot.channels[0].isSilent);
    LSW_CHECK_NEAR(snapshot.channels[0].rmsDbfs, -160.0, 1.0e-9);
}

LSW_TEST_CASE(one_kilohertz_sine_reports_unit_peak)
{
    const auto snapshot = processMono(lsw::audio_diag::test::sine<float>(480U),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_NEAR(snapshot.channels[0].samplePeak, 1.0, 1.0e-5);
}

LSW_TEST_CASE(minus_six_dbfs_sine_has_expected_rms_range)
{
    auto config = lsw::audio_diag::test::quickConfig(1U, 48000U);
    config.levelTimeConstantSeconds = 0.02;
    const auto snapshot = processMono(lsw::audio_diag::test::sine<float>(48000U, 0.5), config);
    LSW_CHECK_NEAR(snapshot.channels[0].smoothedRms, 0.353553, 0.015);
    LSW_CHECK_NEAR(snapshot.channels[0].rmsDbfs, -9.0309, 0.4);
}

LSW_TEST_CASE(full_scale_sine_detects_peak_and_clipping)
{
    const auto snapshot = processMono(lsw::audio_diag::test::sine<float>(480U),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_NEAR(snapshot.channels[0].maximumAbsoluteSample, 1.0, 1.0e-5);
    LSW_CHECK(snapshot.channels[0].clipCount > 0U);
}

LSW_TEST_CASE(samples_below_clip_threshold_do_not_clip)
{
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(32U, 0.999),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 0U);
}

LSW_TEST_CASE(samples_at_clip_threshold_are_counted)
{
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(32U, 1.0),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 32U);
}

LSW_TEST_CASE(samples_over_clip_threshold_are_counted)
{
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(32U, 1.2),
                                      lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 32U);
    LSW_CHECK_NEAR(snapshot.channels[0].maximumAbsoluteSample, 1.2, 1.0e-6);
}

LSW_TEST_CASE(consecutive_clip_count_tracks_current_run)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 8U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const float input[] { 1.0F, 1.0F, 1.0F, 0.5F, 1.0F, 1.0F, 1.0F, 1.0F };
    const float* channels[] { input };
    analyzer.process(channels, 1U, 8U);
    LSW_CHECK_EQ(analyzer.getSnapshot().channels[0].consecutiveClipCount, 4U);
}

LSW_TEST_CASE(peak_is_cleared_by_reset)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U);
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto input = lsw::audio_diag::test::constant<float>(32U, 0.75);
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    analyzer.reset();
    LSW_CHECK_NEAR(analyzer.getSnapshot().channels[0].maximumAbsoluteSample, 0.0, 1.0e-12);
}

LSW_TEST_CASE(rms_smoothing_transitions_between_levels)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 480U);
    config.levelTimeConstantSeconds = 0.01;
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto lower = lsw::audio_diag::test::constant<float>(480U, 0.25);
    const auto higher = lsw::audio_diag::test::constant<float>(480U, 0.75);
    const float* lowerChannels[] { lower.data() };
    const float* higherChannels[] { higher.data() };
    analyzer.process(lowerChannels, 1U, lower.size());
    const double firstRms = analyzer.getSnapshot().channels[0].smoothedRms;
    analyzer.process(higherChannels, 1U, higher.size());
    const double secondRms = analyzer.getSnapshot().channels[0].smoothedRms;
    LSW_CHECK(secondRms > firstRms);
    LSW_CHECK(secondRms < 0.75);
}

LSW_TEST_CASE(sample_rate_44100_hz_is_accepted)
{
    auto config = lsw::audio_diag::test::quickConfig(1U);
    config.sampleRate = 44100.0;
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(16U, 0.25), config);
    LSW_CHECK_NEAR(snapshot.sampleRate, 44100.0, 1.0e-12);
}

LSW_TEST_CASE(sample_rate_48000_hz_is_accepted)
{
    auto config = lsw::audio_diag::test::quickConfig(1U);
    config.sampleRate = 48000.0;
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(16U, 0.25), config);
    LSW_CHECK_NEAR(snapshot.sampleRate, 48000.0, 1.0e-12);
}

LSW_TEST_CASE(sample_rate_88200_hz_is_accepted)
{
    auto config = lsw::audio_diag::test::quickConfig(1U);
    config.sampleRate = 88200.0;
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(16U, 0.25), config);
    LSW_CHECK_NEAR(snapshot.sampleRate, 88200.0, 1.0e-12);
}

LSW_TEST_CASE(sample_rate_96000_hz_is_accepted)
{
    auto config = lsw::audio_diag::test::quickConfig(1U);
    config.sampleRate = 96000.0;
    const auto snapshot = processMono(lsw::audio_diag::test::constant<float>(16U, 0.25), config);
    LSW_CHECK_NEAR(snapshot.sampleRate, 96000.0, 1.0e-12);
}

LSW_TEST_CASE(block_size_one_is_supported)
{
    checkBlockSize(1U);
}

LSW_TEST_CASE(block_size_sixteen_is_supported)
{
    checkBlockSize(16U);
}

LSW_TEST_CASE(block_size_thirty_two_is_supported)
{
    checkBlockSize(32U);
}

LSW_TEST_CASE(block_size_sixty_four_is_supported)
{
    checkBlockSize(64U);
}

LSW_TEST_CASE(block_size_one_twenty_eight_is_supported)
{
    checkBlockSize(128U);
}

LSW_TEST_CASE(block_size_five_twelve_is_supported)
{
    checkBlockSize(512U);
}

LSW_TEST_CASE(block_size_two_thousand_forty_eight_is_supported)
{
    checkBlockSize(2048U);
}
