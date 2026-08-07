// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "lsw/audio_diag/diagnostic_flags.hpp"
#include "lsw/audio_diag/version.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <cstddef>
#include <cstring>

namespace
{
    using lsw::audio_diag::Analyzer;
    using lsw::audio_diag::DiagnosticFlags;
    using lsw::audio_diag::PrepareResult;

    [[nodiscard]] Analyzer<float> preparedMono()
    {
        Analyzer<float> analyzer;
        const PrepareResult result = analyzer.prepare(lsw::audio_diag::test::quickConfig(1U));
        if (!lsw::audio_diag::isSuccess(result))
        {
            return Analyzer<float> {};
        }
        return analyzer;
    }
}

LSW_TEST_CASE(prepare_succeeds_with_default_valid_config)
{
    Analyzer<float> analyzer;
    LSW_CHECK_EQ(analyzer.prepare(lsw::audio_diag::AnalyzerConfig {}), PrepareResult::success);
    LSW_CHECK(analyzer.getSnapshot().prepared);
}

LSW_TEST_CASE(prepare_rejects_invalid_sample_rate)
{
    Analyzer<float> analyzer;
    lsw::audio_diag::AnalyzerConfig config {};
    config.sampleRate = 0.0;
    LSW_CHECK_EQ(analyzer.prepare(config), PrepareResult::invalidSampleRate);
}

LSW_TEST_CASE(prepare_rejects_zero_channel_count)
{
    Analyzer<float> analyzer;
    lsw::audio_diag::AnalyzerConfig config {};
    config.numberOfChannels = 0U;
    LSW_CHECK_EQ(analyzer.prepare(config), PrepareResult::invalidChannelCount);
}

LSW_TEST_CASE(prepare_rejects_more_than_stereo)
{
    Analyzer<float> analyzer;
    lsw::audio_diag::AnalyzerConfig config {};
    config.numberOfChannels = 3U;
    LSW_CHECK_EQ(analyzer.prepare(config), PrepareResult::invalidChannelCount);
}

LSW_TEST_CASE(prepare_rejects_zero_maximum_block_size)
{
    Analyzer<float> analyzer;
    lsw::audio_diag::AnalyzerConfig config {};
    config.maximumBlockSize = 0U;
    LSW_CHECK_EQ(analyzer.prepare(config), PrepareResult::invalidMaximumBlockSize);
}

LSW_TEST_CASE(process_before_prepare_is_safe_and_inert)
{
    Analyzer<float> analyzer;
    const auto input = lsw::audio_diag::test::constant<float>(16U, 0.25);
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(!snapshot.prepared);
    LSW_CHECK_EQ(snapshot.processedSampleCount, 0U);
}

LSW_TEST_CASE(reset_clears_metrics_and_counts)
{
    Analyzer<float> analyzer = preparedMono();
    const auto input = lsw::audio_diag::test::constant<float>(32U, 1.0);
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    analyzer.reset();
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_NEAR(snapshot.channels[0].maximumAbsoluteSample, 0.0, 1.0e-12);
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 0U);
    LSW_CHECK_EQ(snapshot.processedBlockCount, 0U);
}

LSW_TEST_CASE(zero_sample_block_is_counted_safely)
{
    Analyzer<float> analyzer = preparedMono();
    const float* channels[] { nullptr };
    analyzer.process(channels, 1U, 0U);
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.processedSampleCount, 0U);
    LSW_CHECK_EQ(snapshot.processedBlockCount, 1U);
}

LSW_TEST_CASE(null_channel_pointer_is_reported_without_crash)
{
    Analyzer<float> analyzer = preparedMono();
    const float* channels[] { nullptr };
    analyzer.process(channels, 1U, 16U);
    LSW_CHECK(lsw::audio_diag::hasFlag(analyzer.getSnapshot().diagnosticFlags,
                                       DiagnosticFlags::nullInput));
}

LSW_TEST_CASE(null_channel_array_is_reported_without_crash)
{
    Analyzer<float> analyzer = preparedMono();
    analyzer.process(nullptr, 1U, 16U);
    LSW_CHECK(lsw::audio_diag::hasFlag(analyzer.getSnapshot().diagnosticFlags,
                                       DiagnosticFlags::nullInput));
}

LSW_TEST_CASE(channel_count_mismatch_is_reported)
{
    Analyzer<float> analyzer = preparedMono();
    const auto input = lsw::audio_diag::test::constant<float>(16U, 0.25);
    const float* channels[] { input.data(), input.data() };
    analyzer.process(channels, 2U, input.size());
    LSW_CHECK(lsw::audio_diag::hasFlag(analyzer.getSnapshot().diagnosticFlags,
                                       DiagnosticFlags::channelCountMismatch));
}

LSW_TEST_CASE(oversized_block_is_processed_and_reported_safely)
{
    Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 16U);
    LSW_CHECK_EQ(analyzer.prepare(config), PrepareResult::success);
    const auto input = lsw::audio_diag::test::constant<float>(32U, 0.25);
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.processedSampleCount, 32U);
    LSW_CHECK(lsw::audio_diag::hasFlag(snapshot.diagnosticFlags, DiagnosticFlags::blockSizeExceeded));
}

LSW_TEST_CASE(processed_sample_and_block_counts_accumulate)
{
    Analyzer<float> analyzer = preparedMono();
    const auto input = lsw::audio_diag::test::constant<float>(16U, 0.25);
    const float* channels[] { input.data() };
    analyzer.process(channels, 1U, input.size());
    analyzer.process(channels, 1U, input.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.processedSampleCount, 32U);
    LSW_CHECK_EQ(snapshot.processedBlockCount, 2U);
}

namespace
{
    [[nodiscard]] bool sameInt(const int actual, const int expected) noexcept
    {
        return actual == expected;
    }
}

LSW_TEST_CASE(version_constants_and_string_match_v0_3_0)
{
    LSW_CHECK(sameInt(LSW_AUDIO_DIAG_VERSION_MAJOR, 0));
    LSW_CHECK(sameInt(LSW_AUDIO_DIAG_VERSION_MINOR, 3));
    LSW_CHECK(sameInt(LSW_AUDIO_DIAG_VERSION_PATCH, 0));
    LSW_CHECK(sameInt(lsw::audio_diag::versionMajor, 0));
    LSW_CHECK(sameInt(lsw::audio_diag::versionMinor, 3));
    LSW_CHECK(sameInt(lsw::audio_diag::versionPatch, 0));
    LSW_CHECK_EQ(std::strcmp(LSW_AUDIO_DIAG_VERSION_STRING, "0.3.0"), 0);
    LSW_CHECK_EQ(std::strcmp(lsw::audio_diag::versionString, "0.3.0"), 0);
}
