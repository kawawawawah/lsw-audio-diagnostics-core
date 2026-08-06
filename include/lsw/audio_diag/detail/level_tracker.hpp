// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/analyzer_config.hpp"
#include "lsw/audio_diag/channel_metrics.hpp"
#include "lsw/audio_diag/detail/finite_value_guard.hpp"

#include <cstddef>
#include <cstdint>

namespace lsw::audio_diag::detail
{
    class LevelTracker
    {
    public:
        void configure(const AnalyzerConfig& config) noexcept;
        void reset() noexcept;
        void beginBlock() noexcept;
        void processSample(double sample, SampleClassification classification) noexcept;
        void endBlock(std::size_t numberOfSamples) noexcept;
        [[nodiscard]] ChannelMetrics metrics() const noexcept;

    private:
        double levelAlpha_ = 0.0;
        double dcAlpha_ = 0.0;
        double silenceThresholdDbfs_ = -90.0;
        double clipThreshold_ = 1.0;
        std::uint64_t silenceHoldSamples_ = 0U;
        double smoothedMeanSquare_ = 0.0;
        double smoothedDc_ = 0.0;
        double blockPeak_ = 0.0;
        double maximumAbsoluteSample_ = 0.0;
        std::uint64_t clipCount_ = 0U;
        std::uint64_t consecutiveClipCount_ = 0U;
        std::uint64_t invalidSampleCount_ = 0U;
        std::uint64_t nanCount_ = 0U;
        std::uint64_t positiveInfinityCount_ = 0U;
        std::uint64_t negativeInfinityCount_ = 0U;
        std::uint64_t denormalCount_ = 0U;
        std::uint64_t silenceSamples_ = 0U;
        bool isSilent_ = false;
    };
}
