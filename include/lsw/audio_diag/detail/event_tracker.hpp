// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/analyzer_config.hpp"
#include "lsw/audio_diag/channel_metrics.hpp"
#include "lsw/audio_diag/detail/finite_value_guard.hpp"
#include "lsw/audio_diag/event_metrics.hpp"

#include <cstddef>
#include <cstdint>

namespace lsw::audio_diag::detail
{
    class ChannelEventTracker
    {
    public:
        void configure(const AnalyzerConfig& config) noexcept;
        void reset() noexcept;
        void resetLevelDetectors() noexcept;
        void clearEvents() noexcept;
        void beginBlock() noexcept;
        void processSample(bool isClip, SampleClassification classification,
                           std::uint64_t samplePosition) noexcept;
        void endBlock(const ChannelMetrics& metrics, std::size_t numberOfSamples,
                      std::uint64_t blockStartSample) noexcept;
        [[nodiscard]] const ChannelEvents& events() const noexcept;

    private:
        ChannelEvents events_ {};
        double activeSignalThresholdDbfs_ = -60.0;
        double dropoutThresholdDbfs_ = -80.0;
        double dcFaultThreshold_ = 0.01;
        std::uint64_t dropoutHoldSamples_ = 0U;
        std::uint64_t dropoutRecoverySamples_ = 0U;
        std::uint64_t dcFaultHoldSamples_ = 0U;
        std::uint64_t dcFaultRecoverySamples_ = 0U;
        std::uint64_t sustainedClipMinimumSamples_ = 3U;
        std::uint64_t invalidBurstThresholdPerBlock_ = 2U;
        std::uint64_t clipRunSamples_ = 0U;
        std::uint64_t clipRunStartSample_ = 0U;
        std::uint64_t invalidSamplesInBlock_ = 0U;
        std::uint64_t dropoutArmedSamples_ = 0U;
        std::uint64_t dropoutLowSamples_ = 0U;
        std::uint64_t dropoutLowStartSample_ = 0U;
        std::uint64_t dropoutRecoverySamplesAccumulated_ = 0U;
        std::uint64_t dcFaultCandidateSamples_ = 0U;
        std::uint64_t dcFaultCandidateStartSample_ = 0U;
        std::uint64_t dcFaultRecoverySamplesAccumulated_ = 0U;
        bool dropoutArmed_ = false;
    };

    class StereoEventTracker
    {
    public:
        void reset() noexcept;
        void clearEvents() noexcept;
        void update(bool reversedPolarity, bool identicalChannels, bool leftOnly, bool rightOnly,
                    std::size_t numberOfSamples, std::uint64_t blockStartSample) noexcept;
        [[nodiscard]] const StereoEvents& events() const noexcept;

    private:
        StereoEvents events_ {};
    };
}
