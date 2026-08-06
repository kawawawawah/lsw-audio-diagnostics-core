// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

namespace lsw::audio_diag
{
    /** Result returned by Analyzer::prepare. */
    enum class PrepareResult
    {
        success,
        invalidSampleRate,
        invalidMaximumBlockSize,
        invalidChannelCount,
        invalidTimeConstant,
        invalidThreshold,
        invalidTolerance
    };

    [[nodiscard]] constexpr bool isSuccess(const PrepareResult result) noexcept
    {
        return result == PrepareResult::success;
    }

    /** Configuration fixed for one prepared Analyzer instance. */
    struct AnalyzerConfig
    {
        double sampleRate = 48000.0;
        std::size_t maximumBlockSize = 512U;
        std::size_t numberOfChannels = 2U;

        double levelTimeConstantSeconds = 0.300;
        double dcTimeConstantSeconds = 1.000;
        double correlationTimeConstantSeconds = 0.500;

        double silenceThresholdDbfs = -90.0;
        double activeSignalThresholdDbfs = -60.0;
        double clipThreshold = 1.0;

        double identicalChannelTolerance = 1.0e-6;
        double identicalCorrelationThreshold = 0.999;
        double reversedPolarityThreshold = -0.950;

        double silenceHoldSeconds = 0.500;

        double peakHoldSeconds = 2.0;
        double peakHoldDecayDbPerSecond = 12.0;

        double dropoutThresholdDbfs = -80.0;
        double dropoutHoldSeconds = 0.100;
        double dropoutRecoverySeconds = 0.050;

        std::uint64_t sustainedClipMinimumSamples = 3U;

        double dcFaultThreshold = 0.01;
        double dcFaultHoldSeconds = 0.250;
        double dcFaultRecoverySeconds = 0.250;

        std::uint64_t invalidBurstThresholdPerBlock = 2U;
    };
}
