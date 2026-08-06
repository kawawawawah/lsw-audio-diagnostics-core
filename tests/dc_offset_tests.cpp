// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <vector>

namespace
{
    [[nodiscard]] lsw::audio_diag::Snapshot processDc(const std::vector<float>& input,
                                                       lsw::audio_diag::AnalyzerConfig config)
    {
        config.numberOfChannels = 1U;
        config.maximumBlockSize = input.size();
        lsw::audio_diag::Analyzer<float> analyzer;
        if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
        {
            return {};
        }
        const float* channels[] { input.data() };
        analyzer.process(channels, 1U, input.size());
        return analyzer.getSnapshot();
    }
}

LSW_TEST_CASE(positive_dc_offset_is_measured)
{
    const auto snapshot = processDc(lsw::audio_diag::test::constant<float>(128U, 0.25),
                                    lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_NEAR(snapshot.channels[0].dcOffset, 0.25, 1.0e-6);
}

LSW_TEST_CASE(negative_dc_offset_is_measured)
{
    const auto snapshot = processDc(lsw::audio_diag::test::constant<float>(128U, -0.25),
                                    lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_NEAR(snapshot.channels[0].dcOffset, -0.25, 1.0e-6);
}

LSW_TEST_CASE(zero_dc_offset_remains_zero)
{
    const auto snapshot = processDc(lsw::audio_diag::test::silence<float>(128U),
                                    lsw::audio_diag::test::quickConfig(1U));
    LSW_CHECK_NEAR(snapshot.channels[0].dcOffset, 0.0, 1.0e-12);
}

LSW_TEST_CASE(dc_offset_is_smoothed_between_blocks)
{
    lsw::audio_diag::Analyzer<float> analyzer;
    auto config = lsw::audio_diag::test::quickConfig(1U, 480U);
    config.dcTimeConstantSeconds = 0.01;
    LSW_CHECK(lsw::audio_diag::isSuccess(analyzer.prepare(config)));
    const auto silence = lsw::audio_diag::test::silence<float>(480U);
    const auto offset = lsw::audio_diag::test::constant<float>(480U, 0.5);
    const float* silenceChannels[] { silence.data() };
    const float* offsetChannels[] { offset.data() };
    analyzer.process(silenceChannels, 1U, silence.size());
    analyzer.process(offsetChannels, 1U, offset.size());
    const double dc = analyzer.getSnapshot().channels[0].dcOffset;
    LSW_CHECK(dc > 0.0);
    LSW_CHECK(dc < 0.5);
}

LSW_TEST_CASE(sine_wave_has_near_zero_smoothed_dc_offset)
{
    auto config = lsw::audio_diag::test::quickConfig(1U, 96000U);
    config.dcTimeConstantSeconds = 0.05;
    const auto snapshot = processDc(lsw::audio_diag::test::sine<float>(96000U), config);
    LSW_CHECK_NEAR(snapshot.channels[0].dcOffset, 0.0, 0.02);
}
