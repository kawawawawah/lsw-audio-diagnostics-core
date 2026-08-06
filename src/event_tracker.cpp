// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/detail/event_tracker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lsw::audio_diag::detail
{
    namespace
    {
        [[nodiscard]] std::uint64_t addSaturated(const std::uint64_t current,
                                                  const std::uint64_t addition) noexcept
        {
            const std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
            return addition > (maximum - current) ? maximum : current + addition;
        }

        [[nodiscard]] std::uint64_t secondsToSamples(const double seconds,
                                                      const double sampleRate) noexcept
        {
            const double requested = std::ceil(seconds * sampleRate);
            const double maximum = static_cast<double>(std::numeric_limits<std::uint64_t>::max());
            return requested >= maximum ? std::numeric_limits<std::uint64_t>::max()
                                        : static_cast<std::uint64_t>(requested);
        }

        void advance(EventState& state, const std::uint64_t numberOfSamples) noexcept
        {
            state.currentDurationSamples = addSaturated(state.currentDurationSamples, numberOfSamples);
            state.longestDurationSamples = std::max(state.longestDurationSamples,
                                                    state.currentDurationSamples);
        }

        void start(EventState& state, const std::uint64_t startSample,
                   const std::uint64_t initialDuration) noexcept
        {
            state.active = true;
            state.latched = true;
            state.eventCount = addSaturated(state.eventCount, 1U);
            state.currentDurationSamples = initialDuration;
            state.longestDurationSamples = std::max(state.longestDurationSamples,
                                                    initialDuration);
            state.lastStartedAtSample = startSample;
        }

        void finish(EventState& state) noexcept
        {
            state.active = false;
            state.currentDurationSamples = 0U;
        }

        void updateBlockEvent(EventState& state, const bool condition,
                              const std::uint64_t numberOfSamples,
                              const std::uint64_t blockStartSample) noexcept
        {
            if (condition)
            {
                if (state.active)
                {
                    advance(state, numberOfSamples);
                }
                else
                {
                    start(state, blockStartSample, numberOfSamples);
                }
            }
            else
            {
                finish(state);
            }
        }

        void clearHistory(EventState& state) noexcept
        {
            state.latched = false;
            state.eventCount = 0U;
            state.longestDurationSamples = 0U;
            state.lastStartedAtSample = 0U;
        }
    }

    void ChannelEventTracker::configure(const AnalyzerConfig& config) noexcept
    {
        activeSignalThresholdDbfs_ = config.activeSignalThresholdDbfs;
        dropoutThresholdDbfs_ = config.dropoutThresholdDbfs;
        dcFaultThreshold_ = config.dcFaultThreshold;
        dropoutHoldSamples_ = secondsToSamples(config.dropoutHoldSeconds, config.sampleRate);
        dropoutRecoverySamples_ = secondsToSamples(config.dropoutRecoverySeconds, config.sampleRate);
        dcFaultHoldSamples_ = secondsToSamples(config.dcFaultHoldSeconds, config.sampleRate);
        dcFaultRecoverySamples_ = secondsToSamples(config.dcFaultRecoverySeconds, config.sampleRate);
        sustainedClipMinimumSamples_ = config.sustainedClipMinimumSamples;
        invalidBurstThresholdPerBlock_ = config.invalidBurstThresholdPerBlock;
        reset();
    }

    void ChannelEventTracker::reset() noexcept
    {
        events_ = {};
        clipRunSamples_ = 0U;
        clipRunStartSample_ = 0U;
        invalidSamplesInBlock_ = 0U;
        dropoutArmedSamples_ = 0U;
        dropoutLowSamples_ = 0U;
        dropoutLowStartSample_ = 0U;
        dropoutRecoverySamplesAccumulated_ = 0U;
        dcFaultCandidateSamples_ = 0U;
        dcFaultCandidateStartSample_ = 0U;
        dcFaultRecoverySamplesAccumulated_ = 0U;
        dropoutArmed_ = false;
    }

    void ChannelEventTracker::resetLevelDetectors() noexcept
    {
        dropoutArmedSamples_ = 0U;
        dropoutLowSamples_ = 0U;
        dropoutLowStartSample_ = 0U;
        dropoutRecoverySamplesAccumulated_ = 0U;
        dcFaultCandidateSamples_ = 0U;
        dcFaultCandidateStartSample_ = 0U;
        dcFaultRecoverySamplesAccumulated_ = 0U;
        dropoutArmed_ = false;
        finish(events_.dropout);
        finish(events_.dcFault);
    }

    void ChannelEventTracker::clearEvents() noexcept
    {
        clearHistory(events_.dropout);
        clearHistory(events_.sustainedClip);
        clearHistory(events_.dcFault);
        clearHistory(events_.invalidBurst);
        events_.maximumObservedDcOffset = 0.0;
        events_.maximumInvalidSamplesPerBlock = 0U;
    }

    void ChannelEventTracker::beginBlock() noexcept
    {
        invalidSamplesInBlock_ = 0U;
    }

    void ChannelEventTracker::processSample(const bool isClip,
                                             const SampleClassification classification,
                                             const std::uint64_t samplePosition) noexcept
    {
        if (classification == SampleClassification::nan
            || classification == SampleClassification::positiveInfinity
            || classification == SampleClassification::negativeInfinity)
        {
            invalidSamplesInBlock_ = addSaturated(invalidSamplesInBlock_, 1U);
        }

        EventState& clip = events_.sustainedClip;
        if (!isClip)
        {
            clipRunSamples_ = 0U;
            finish(clip);
            return;
        }

        if (clipRunSamples_ == 0U)
        {
            clipRunStartSample_ = samplePosition;
        }
        clipRunSamples_ = addSaturated(clipRunSamples_, 1U);

        if (clip.active)
        {
            clip.currentDurationSamples = clipRunSamples_;
            clip.longestDurationSamples = std::max(clip.longestDurationSamples, clipRunSamples_);
        }
        else if (clipRunSamples_ >= sustainedClipMinimumSamples_)
        {
            start(clip, clipRunStartSample_, clipRunSamples_);
        }
    }

    void ChannelEventTracker::endBlock(const ChannelMetrics& metrics,
                                       const std::size_t numberOfSamples,
                                       const std::uint64_t blockStartSample) noexcept
    {
        const std::uint64_t blockSamples = static_cast<std::uint64_t>(numberOfSamples);
        events_.maximumInvalidSamplesPerBlock = std::max(events_.maximumInvalidSamplesPerBlock,
                                                         invalidSamplesInBlock_);
        updateBlockEvent(events_.invalidBurst,
                         invalidSamplesInBlock_ >= invalidBurstThresholdPerBlock_,
                         blockSamples, blockStartSample);

        const bool activeSignal = metrics.rmsDbfs >= activeSignalThresholdDbfs_;
        const bool dropoutLow = metrics.rmsDbfs < dropoutThresholdDbfs_;
        if (activeSignal)
        {
            dropoutArmedSamples_ = addSaturated(dropoutArmedSamples_, blockSamples);
            if (dropoutArmedSamples_ >= dropoutRecoverySamples_)
            {
                dropoutArmed_ = true;
            }
        }

        EventState& dropout = events_.dropout;
        if (dropout.active)
        {
            advance(dropout, blockSamples);
            dropoutRecoverySamplesAccumulated_ = activeSignal
                                                     ? addSaturated(dropoutRecoverySamplesAccumulated_, blockSamples)
                                                     : 0U;
            if (dropoutRecoverySamplesAccumulated_ >= dropoutRecoverySamples_ && activeSignal)
            {
                finish(dropout);
                dropoutRecoverySamplesAccumulated_ = 0U;
            }
        }
        else if (dropoutArmed_ && dropoutLow)
        {
            if (dropoutLowSamples_ == 0U)
            {
                dropoutLowStartSample_ = blockStartSample;
            }
            dropoutLowSamples_ = addSaturated(dropoutLowSamples_, blockSamples);
            if (dropoutLowSamples_ >= dropoutHoldSamples_)
            {
                start(dropout, dropoutLowStartSample_, dropoutLowSamples_);
                dropoutLowSamples_ = 0U;
            }
        }
        else
        {
            dropoutLowSamples_ = 0U;
        }

        const double absoluteDcOffset = std::abs(metrics.dcOffset);
        events_.maximumObservedDcOffset = std::max(events_.maximumObservedDcOffset, absoluteDcOffset);
        const bool dcFaultCondition = activeSignal && absoluteDcOffset >= dcFaultThreshold_;
        EventState& dcFault = events_.dcFault;
        if (dcFault.active)
        {
            advance(dcFault, blockSamples);
            dcFaultRecoverySamplesAccumulated_ = !dcFaultCondition
                                                      ? addSaturated(dcFaultRecoverySamplesAccumulated_, blockSamples)
                                                      : 0U;
            if (!dcFaultCondition && dcFaultRecoverySamplesAccumulated_ >= dcFaultRecoverySamples_)
            {
                finish(dcFault);
                dcFaultRecoverySamplesAccumulated_ = 0U;
            }
        }
        else if (dcFaultCondition)
        {
            if (dcFaultCandidateSamples_ == 0U)
            {
                dcFaultCandidateStartSample_ = blockStartSample;
            }
            dcFaultCandidateSamples_ = addSaturated(dcFaultCandidateSamples_, blockSamples);
            if (dcFaultCandidateSamples_ >= dcFaultHoldSamples_)
            {
                start(dcFault, dcFaultCandidateStartSample_, dcFaultCandidateSamples_);
                dcFaultCandidateSamples_ = 0U;
            }
        }
        else
        {
            dcFaultCandidateSamples_ = 0U;
        }
    }

    const ChannelEvents& ChannelEventTracker::events() const noexcept
    {
        return events_;
    }

    void StereoEventTracker::reset() noexcept
    {
        events_ = {};
    }

    void StereoEventTracker::clearEvents() noexcept
    {
        clearHistory(events_.reversedPolarity);
        clearHistory(events_.identicalChannels);
        clearHistory(events_.leftOnly);
        clearHistory(events_.rightOnly);
    }

    void StereoEventTracker::update(const bool reversedPolarity, const bool identicalChannels,
                                    const bool leftOnly, const bool rightOnly,
                                    const std::size_t numberOfSamples,
                                    const std::uint64_t blockStartSample) noexcept
    {
        const std::uint64_t blockSamples = static_cast<std::uint64_t>(numberOfSamples);
        updateBlockEvent(events_.reversedPolarity, reversedPolarity, blockSamples, blockStartSample);
        updateBlockEvent(events_.identicalChannels, identicalChannels, blockSamples, blockStartSample);
        updateBlockEvent(events_.leftOnly, leftOnly, blockSamples, blockStartSample);
        updateBlockEvent(events_.rightOnly, rightOnly, blockSamples, blockStartSample);
    }

    const StereoEvents& StereoEventTracker::events() const noexcept
    {
        return events_;
    }
}
