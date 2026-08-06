// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace lsw::audio_diag::detail
{
    namespace
    {
        constexpr std::uint32_t identicalBit = 1U << 0U;
        constexpr std::uint32_t reversedBit = 1U << 1U;
        constexpr std::uint32_t leftOnlyBit = 1U << 2U;
        constexpr std::uint32_t rightOnlyBit = 1U << 3U;

        [[nodiscard]] std::uint32_t asUint(const bool value) noexcept
        {
            return value ? 1U : 0U;
        }
    }

    void AtomicChannelMetrics::store(const ChannelMetrics& value) noexcept
    {
        samplePeak_.store(doubleToBits(value.samplePeak), std::memory_order_seq_cst);
        heldPeak_.store(doubleToBits(value.heldPeak), std::memory_order_seq_cst);
        heldPeakDbfs_.store(doubleToBits(value.heldPeakDbfs), std::memory_order_seq_cst);
        smoothedRms_.store(doubleToBits(value.smoothedRms), std::memory_order_seq_cst);
        rmsDbfs_.store(doubleToBits(value.rmsDbfs), std::memory_order_seq_cst);
        dcOffset_.store(doubleToBits(value.dcOffset), std::memory_order_seq_cst);
        maximumAbsoluteSample_.store(doubleToBits(value.maximumAbsoluteSample), std::memory_order_seq_cst);
        clipCount_.store(value.clipCount, std::memory_order_seq_cst);
        consecutiveClipCount_.store(value.consecutiveClipCount, std::memory_order_seq_cst);
        invalidSampleCount_.store(value.invalidSampleCount, std::memory_order_seq_cst);
        nanCount_.store(value.nanCount, std::memory_order_seq_cst);
        positiveInfinityCount_.store(value.positiveInfinityCount, std::memory_order_seq_cst);
        negativeInfinityCount_.store(value.negativeInfinityCount, std::memory_order_seq_cst);
        denormalCount_.store(value.denormalCount, std::memory_order_seq_cst);
        isSilent_.store(asUint(value.isSilent), std::memory_order_seq_cst);
    }

    ChannelMetrics AtomicChannelMetrics::load() const noexcept
    {
        ChannelMetrics value {};
        value.samplePeak = bitsToDouble(samplePeak_.load(std::memory_order_seq_cst));
        value.heldPeak = bitsToDouble(heldPeak_.load(std::memory_order_seq_cst));
        value.heldPeakDbfs = bitsToDouble(heldPeakDbfs_.load(std::memory_order_seq_cst));
        value.smoothedRms = bitsToDouble(smoothedRms_.load(std::memory_order_seq_cst));
        value.rmsDbfs = bitsToDouble(rmsDbfs_.load(std::memory_order_seq_cst));
        value.dcOffset = bitsToDouble(dcOffset_.load(std::memory_order_seq_cst));
        value.maximumAbsoluteSample = bitsToDouble(maximumAbsoluteSample_.load(std::memory_order_seq_cst));
        value.clipCount = clipCount_.load(std::memory_order_seq_cst);
        value.consecutiveClipCount = consecutiveClipCount_.load(std::memory_order_seq_cst);
        value.invalidSampleCount = invalidSampleCount_.load(std::memory_order_seq_cst);
        value.nanCount = nanCount_.load(std::memory_order_seq_cst);
        value.positiveInfinityCount = positiveInfinityCount_.load(std::memory_order_seq_cst);
        value.negativeInfinityCount = negativeInfinityCount_.load(std::memory_order_seq_cst);
        value.denormalCount = denormalCount_.load(std::memory_order_seq_cst);
        value.isSilent = isSilent_.load(std::memory_order_seq_cst) != 0U;
        return value;
    }

    void AtomicEventState::store(const EventState& value) noexcept
    {
        std::uint32_t state = 0U;
        if (value.active)
        {
            state |= 1U;
        }
        if (value.latched)
        {
            state |= 2U;
        }
        state_.store(state, std::memory_order_seq_cst);
        eventCount_.store(value.eventCount, std::memory_order_seq_cst);
        currentDurationSamples_.store(value.currentDurationSamples, std::memory_order_seq_cst);
        longestDurationSamples_.store(value.longestDurationSamples, std::memory_order_seq_cst);
        lastStartedAtSample_.store(value.lastStartedAtSample, std::memory_order_seq_cst);
    }

    EventState AtomicEventState::load() const noexcept
    {
        EventState value {};
        const std::uint32_t state = state_.load(std::memory_order_seq_cst);
        value.active = (state & 1U) != 0U;
        value.latched = (state & 2U) != 0U;
        value.eventCount = eventCount_.load(std::memory_order_seq_cst);
        value.currentDurationSamples = currentDurationSamples_.load(std::memory_order_seq_cst);
        value.longestDurationSamples = longestDurationSamples_.load(std::memory_order_seq_cst);
        value.lastStartedAtSample = lastStartedAtSample_.load(std::memory_order_seq_cst);
        return value;
    }

    void AtomicChannelEvents::store(const ChannelEvents& value) noexcept
    {
        dropout_.store(value.dropout);
        sustainedClip_.store(value.sustainedClip);
        dcFault_.store(value.dcFault);
        invalidBurst_.store(value.invalidBurst);
        maximumObservedDcOffset_.store(doubleToBits(value.maximumObservedDcOffset),
                                       std::memory_order_seq_cst);
        maximumInvalidSamplesPerBlock_.store(value.maximumInvalidSamplesPerBlock,
                                             std::memory_order_seq_cst);
    }

    ChannelEvents AtomicChannelEvents::load() const noexcept
    {
        ChannelEvents value {};
        value.dropout = dropout_.load();
        value.sustainedClip = sustainedClip_.load();
        value.dcFault = dcFault_.load();
        value.invalidBurst = invalidBurst_.load();
        value.maximumObservedDcOffset = bitsToDouble(
            maximumObservedDcOffset_.load(std::memory_order_seq_cst));
        value.maximumInvalidSamplesPerBlock = maximumInvalidSamplesPerBlock_.load(
            std::memory_order_seq_cst);
        return value;
    }

    void AtomicStereoEvents::store(const StereoEvents& value) noexcept
    {
        reversedPolarity_.store(value.reversedPolarity);
        identicalChannels_.store(value.identicalChannels);
        leftOnly_.store(value.leftOnly);
        rightOnly_.store(value.rightOnly);
    }

    StereoEvents AtomicStereoEvents::load() const noexcept
    {
        StereoEvents value {};
        value.reversedPolarity = reversedPolarity_.load();
        value.identicalChannels = identicalChannels_.load();
        value.leftOnly = leftOnly_.load();
        value.rightOnly = rightOnly_.load();
        return value;
    }

    void AtomicStereoMetrics::store(const StereoMetrics& value) noexcept
    {
        correlation_.store(doubleToBits(value.correlation), std::memory_order_seq_cst);
        leftRms_.store(doubleToBits(value.leftRms), std::memory_order_seq_cst);
        rightRms_.store(doubleToBits(value.rightRms), std::memory_order_seq_cst);
        channelBalanceDb_.store(doubleToBits(value.channelBalanceDb), std::memory_order_seq_cst);
        monoCompatibilityScore_.store(doubleToBits(value.monoCompatibilityScore), std::memory_order_seq_cst);

        std::uint32_t state = 0U;
        if (value.identicalChannels)
        {
            state |= identicalBit;
        }
        if (value.reversedPolarity)
        {
            state |= reversedBit;
        }
        if (value.leftOnly)
        {
            state |= leftOnlyBit;
        }
        if (value.rightOnly)
        {
            state |= rightOnlyBit;
        }
        state_.store(state, std::memory_order_seq_cst);
    }

    StereoMetrics AtomicStereoMetrics::load() const noexcept
    {
        StereoMetrics value {};
        value.correlation = bitsToDouble(correlation_.load(std::memory_order_seq_cst));
        value.leftRms = bitsToDouble(leftRms_.load(std::memory_order_seq_cst));
        value.rightRms = bitsToDouble(rightRms_.load(std::memory_order_seq_cst));
        value.channelBalanceDb = bitsToDouble(channelBalanceDb_.load(std::memory_order_seq_cst));
        value.monoCompatibilityScore = bitsToDouble(monoCompatibilityScore_.load(std::memory_order_seq_cst));
        const std::uint32_t state = state_.load(std::memory_order_seq_cst);
        value.identicalChannels = (state & identicalBit) != 0U;
        value.reversedPolarity = (state & reversedBit) != 0U;
        value.leftOnly = (state & leftOnlyBit) != 0U;
        value.rightOnly = (state & rightOnlyBit) != 0U;
        return value;
    }

    void AtomicSnapshot::store(const Snapshot& value) noexcept
    {
        sequence_.fetch_add(1U, std::memory_order_seq_cst);
        prepared_.store(asUint(value.prepared), std::memory_order_seq_cst);
        activeChannelCount_.store(static_cast<std::uint64_t>(value.activeChannelCount),
                                  std::memory_order_seq_cst);
        sampleRate_.store(doubleToBits(value.sampleRate), std::memory_order_seq_cst);
        processedSampleCount_.store(value.processedSampleCount, std::memory_order_seq_cst);
        processedBlockCount_.store(value.processedBlockCount, std::memory_order_seq_cst);
        diagnosticFlags_.store(static_cast<std::uint32_t>(value.diagnosticFlags), std::memory_order_seq_cst);
        channels_[0].store(value.channels[0]);
        channels_[1].store(value.channels[1]);
        stereo_.store(value.stereo);
        channelEvents_[0].store(value.channelEvents[0]);
        channelEvents_[1].store(value.channelEvents[1]);
        stereoEvents_.store(value.stereoEvents);
        sequence_.fetch_add(1U, std::memory_order_seq_cst);
    }

    Snapshot AtomicSnapshot::load() const noexcept
    {
        for (;;)
        {
            const std::uint64_t before = sequence_.load(std::memory_order_seq_cst);
            if ((before & 1U) != 0U)
            {
                continue;
            }

            Snapshot value {};
            value.prepared = prepared_.load(std::memory_order_seq_cst) != 0U;
            value.activeChannelCount = static_cast<std::size_t>(activeChannelCount_.load(std::memory_order_seq_cst));
            value.sampleRate = bitsToDouble(sampleRate_.load(std::memory_order_seq_cst));
            value.processedSampleCount = processedSampleCount_.load(std::memory_order_seq_cst);
            value.processedBlockCount = processedBlockCount_.load(std::memory_order_seq_cst);
            value.diagnosticFlags = static_cast<DiagnosticFlags>(diagnosticFlags_.load(std::memory_order_seq_cst));
            value.channels[0] = channels_[0].load();
            value.channels[1] = channels_[1].load();
            value.stereo = stereo_.load();
            value.channelEvents[0] = channelEvents_[0].load();
            value.channelEvents[1] = channelEvents_[1].load();
            value.stereoEvents = stereoEvents_.load();

            if (sequence_.load(std::memory_order_seq_cst) == before)
            {
                return value;
            }
        }
    }
}

namespace lsw::audio_diag
{
    namespace
    {
        constexpr std::size_t maximumSupportedChannels = 2U;

        [[nodiscard]] bool isFinite(const double value) noexcept
        {
            return std::isfinite(value);
        }

        [[nodiscard]] bool isValidCorrelation(const double value) noexcept
        {
            return isFinite(value) && value >= -1.0 && value <= 1.0;
        }

        [[nodiscard]] StereoMetrics makeStereoMetrics(const AnalyzerConfig& config,
                                                       const ChannelMetrics& left,
                                                       const ChannelMetrics& right,
                                                       const detail::CorrelationTracker& tracker) noexcept
        {
            StereoMetrics result {};
            result.correlation = tracker.correlation();
            result.leftRms = left.smoothedRms;
            result.rightRms = right.smoothedRms;
            result.channelBalanceDb = left.rmsDbfs - right.rmsDbfs;
            result.monoCompatibilityScore = std::max(
                0.0, std::min(1.0, (result.correlation + 1.0) * 0.5));

            const bool leftActive = left.rmsDbfs >= config.activeSignalThresholdDbfs;
            const bool rightActive = right.rmsDbfs >= config.activeSignalThresholdDbfs;
            const bool rightSilent = right.rmsDbfs < config.silenceThresholdDbfs;
            const bool leftSilent = left.rmsDbfs < config.silenceThresholdDbfs;
            const bool correlationAvailable = tracker.isAvailable();

            result.identicalChannels = leftActive && rightActive && correlationAvailable
                                       && result.correlation >= config.identicalCorrelationThreshold
                                       && tracker.maximumAbsoluteDifference()
                                              <= config.identicalChannelTolerance;
            result.reversedPolarity = leftActive && rightActive && correlationAvailable
                                      && result.correlation <= config.reversedPolarityThreshold;
            result.leftOnly = leftActive && rightSilent;
            result.rightOnly = rightActive && leftSilent;
            return result;
        }

        [[nodiscard]] PrepareResult validateConfig(const AnalyzerConfig& config) noexcept
        {
            if (!isFinite(config.sampleRate) || config.sampleRate <= 0.0)
            {
                return PrepareResult::invalidSampleRate;
            }
            if (config.maximumBlockSize == 0U)
            {
                return PrepareResult::invalidMaximumBlockSize;
            }
            if (config.numberOfChannels == 0U || config.numberOfChannels > maximumSupportedChannels)
            {
                return PrepareResult::invalidChannelCount;
            }
            if (!isFinite(config.levelTimeConstantSeconds) || config.levelTimeConstantSeconds <= 0.0
                || !isFinite(config.dcTimeConstantSeconds) || config.dcTimeConstantSeconds <= 0.0
                || !isFinite(config.correlationTimeConstantSeconds) || config.correlationTimeConstantSeconds <= 0.0
                || !isFinite(config.silenceHoldSeconds) || config.silenceHoldSeconds < 0.0
                || !isFinite(config.peakHoldSeconds) || config.peakHoldSeconds < 0.0
                || !isFinite(config.dropoutHoldSeconds) || config.dropoutHoldSeconds < 0.0
                || !isFinite(config.dropoutRecoverySeconds) || config.dropoutRecoverySeconds < 0.0
                || !isFinite(config.dcFaultHoldSeconds) || config.dcFaultHoldSeconds < 0.0
                || !isFinite(config.dcFaultRecoverySeconds) || config.dcFaultRecoverySeconds < 0.0)
            {
                return PrepareResult::invalidTimeConstant;
            }
            if (!isFinite(config.silenceThresholdDbfs) || !isFinite(config.activeSignalThresholdDbfs)
                || !isFinite(config.clipThreshold) || config.clipThreshold <= 0.0
                || !isFinite(config.dropoutThresholdDbfs)
                || !isFinite(config.dcFaultThreshold) || config.dcFaultThreshold <= 0.0
                || !isFinite(config.peakHoldDecayDbPerSecond) || config.peakHoldDecayDbPerSecond <= 0.0
                || config.silenceThresholdDbfs > config.dropoutThresholdDbfs
                || config.dropoutThresholdDbfs >= config.activeSignalThresholdDbfs)
            {
                return PrepareResult::invalidThreshold;
            }
            if (!isFinite(config.identicalChannelTolerance) || config.identicalChannelTolerance < 0.0
                || !isValidCorrelation(config.identicalCorrelationThreshold)
                || !isValidCorrelation(config.reversedPolarityThreshold))
            {
                return PrepareResult::invalidTolerance;
            }
            if (config.sustainedClipMinimumSamples == 0U || config.invalidBurstThresholdPerBlock == 0U)
            {
                return PrepareResult::invalidThreshold;
            }
            return PrepareResult::success;
        }

        [[nodiscard]] std::uint64_t addSaturated(const std::uint64_t current,
                                                  const std::size_t addition) noexcept
        {
            const std::uint64_t addend = static_cast<std::uint64_t>(addition);
            const std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
            return addend > (maximum - current) ? maximum : current + addend;
        }
    }

    template <typename SampleType>
    Analyzer<SampleType>::Analyzer() noexcept
    {
        publishSnapshot();
    }

    template <typename SampleType>
    Analyzer<SampleType>::Analyzer(Analyzer&& other) noexcept
    {
        moveFrom(std::move(other));
    }

    template <typename SampleType>
    Analyzer<SampleType>& Analyzer<SampleType>::operator=(Analyzer&& other) noexcept
    {
        if (this != &other)
        {
            moveFrom(std::move(other));
        }
        return *this;
    }

    template <typename SampleType>
    void Analyzer<SampleType>::moveFrom(Analyzer&& other) noexcept
    {
        config_ = other.config_;
        levelTrackers_[0] = other.levelTrackers_[0];
        levelTrackers_[1] = other.levelTrackers_[1];
        eventTrackers_[0] = other.eventTrackers_[0];
        eventTrackers_[1] = other.eventTrackers_[1];
        correlationTracker_ = other.correlationTracker_;
        stereoEventTracker_ = other.stereoEventTracker_;
        prepared_ = other.prepared_;
        processedSampleCount_ = other.processedSampleCount_;
        processedBlockCount_ = other.processedBlockCount_;
        stickyFlags_ = other.stickyFlags_;
        atomicSnapshot_.store(other.getSnapshot());

        other.prepared_ = false;
        other.processedSampleCount_ = 0U;
        other.processedBlockCount_ = 0U;
        other.stickyFlags_ = 0U;
        other.levelTrackers_[0].reset();
        other.levelTrackers_[1].reset();
        other.eventTrackers_[0].reset();
        other.eventTrackers_[1].reset();
        other.correlationTracker_.reset();
        other.stereoEventTracker_.reset();
        other.publishSnapshot();
    }

    template <typename SampleType>
    PrepareResult Analyzer<SampleType>::prepare(const AnalyzerConfig& config) noexcept
    {
        const PrepareResult result = validateConfig(config);
        if (!isSuccess(result))
        {
            return result;
        }

        config_ = config;
        prepared_ = true;
        reset();
        return PrepareResult::success;
    }

    template <typename SampleType>
    void Analyzer<SampleType>::reset() noexcept
    {
        levelTrackers_[0].configure(config_);
        levelTrackers_[1].configure(config_);
        eventTrackers_[0].configure(config_);
        eventTrackers_[1].configure(config_);
        correlationTracker_.configure(config_);
        stereoEventTracker_.reset();
        processedSampleCount_ = 0U;
        processedBlockCount_ = 0U;
        stickyFlags_ = 0U;
        publishSnapshot();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::resetLevels() noexcept
    {
        if (!prepared_)
        {
            return;
        }
        levelTrackers_[0].resetLevels();
        levelTrackers_[1].resetLevels();
        eventTrackers_[0].resetLevelDetectors();
        eventTrackers_[1].resetLevelDetectors();
        correlationTracker_.reset();
        publishSnapshot();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::resetCounters() noexcept
    {
        if (!prepared_)
        {
            return;
        }
        levelTrackers_[0].resetCounters();
        levelTrackers_[1].resetCounters();
        publishSnapshot();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::clearDiagnosticFlags() noexcept
    {
        if (!prepared_)
        {
            return;
        }
        stickyFlags_ = 0U;
        publishSnapshot();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::clearEvents() noexcept
    {
        if (!prepared_)
        {
            return;
        }
        eventTrackers_[0].clearEvents();
        eventTrackers_[1].clearEvents();
        stereoEventTracker_.clearEvents();
        publishSnapshot();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::process(const SampleType* const* channels,
                                       const std::size_t numberOfChannels,
                                       const std::size_t numberOfSamples) noexcept
    {
        if (!prepared_)
        {
            return;
        }
        if (numberOfChannels != config_.numberOfChannels)
        {
            stickyFlags_ |= static_cast<std::uint32_t>(DiagnosticFlags::channelCountMismatch);
        }
        if (numberOfSamples > config_.maximumBlockSize)
        {
            stickyFlags_ |= static_cast<std::uint32_t>(DiagnosticFlags::blockSizeExceeded);
        }
        if (channels == nullptr)
        {
            stickyFlags_ |= static_cast<std::uint32_t>(DiagnosticFlags::nullInput);
            publishSnapshot();
            return;
        }

        bool validInput = numberOfSamples != 0U;
        for (std::size_t channel = 0U; channel < config_.numberOfChannels; ++channel)
        {
            if (channel >= numberOfChannels || channels[channel] == nullptr)
            {
                stickyFlags_ |= static_cast<std::uint32_t>(DiagnosticFlags::nullInput);
                validInput = false;
            }
        }
        if (!validInput)
        {
            if (numberOfSamples == 0U)
            {
                processedBlockCount_ = addSaturated(processedBlockCount_, 1U);
            }
            publishSnapshot();
            return;
        }

        const SampleType* inputs[maximumSupportedChannels] { nullptr, nullptr };
        for (std::size_t channel = 0U; channel < config_.numberOfChannels; ++channel)
        {
            inputs[channel] = channels[channel];
            levelTrackers_[channel].beginBlock();
            eventTrackers_[channel].beginBlock();
        }
        if (config_.numberOfChannels == maximumSupportedChannels)
        {
            correlationTracker_.beginBlock();
        }

        const std::uint64_t blockStartSample = processedSampleCount_;
        for (std::size_t sampleIndex = 0U; sampleIndex < numberOfSamples; ++sampleIndex)
        {
            double samples[maximumSupportedChannels] { 0.0, 0.0 };
            detail::SampleClassification classifications[maximumSupportedChannels] {
                detail::SampleClassification::finite,
                detail::SampleClassification::finite
            };

            for (std::size_t channel = 0U; channel < config_.numberOfChannels; ++channel)
            {
                if (inputs[channel] != nullptr)
                {
                    const detail::SanitizedSample sanitized = detail::sanitizeSample(inputs[channel][sampleIndex]);
                    samples[channel] = sanitized.value;
                    classifications[channel] = sanitized.classification;
                }
                levelTrackers_[channel].processSample(samples[channel], classifications[channel]);
                eventTrackers_[channel].processSample(
                    std::abs(samples[channel]) >= config_.clipThreshold, classifications[channel],
                    addSaturated(blockStartSample, sampleIndex));
            }
            if (config_.numberOfChannels == maximumSupportedChannels)
            {
                correlationTracker_.processPair(samples[0], samples[1]);
            }
        }

        for (std::size_t channel = 0U; channel < config_.numberOfChannels; ++channel)
        {
            levelTrackers_[channel].endBlock(numberOfSamples);
            eventTrackers_[channel].endBlock(levelTrackers_[channel].metrics(), numberOfSamples,
                                             blockStartSample);
        }
        if (config_.numberOfChannels == maximumSupportedChannels)
        {
            const StereoMetrics stereo = makeStereoMetrics(config_, levelTrackers_[0].metrics(),
                                                            levelTrackers_[1].metrics(),
                                                            correlationTracker_);
            stereoEventTracker_.update(stereo.reversedPolarity, stereo.identicalChannels,
                                       stereo.leftOnly, stereo.rightOnly, numberOfSamples,
                                       blockStartSample);
        }
        processedSampleCount_ = addSaturated(processedSampleCount_, numberOfSamples);
        processedBlockCount_ = addSaturated(processedBlockCount_, 1U);
        publishSnapshot();
    }

    template <typename SampleType>
    Snapshot Analyzer<SampleType>::getSnapshot() const noexcept
    {
        return atomicSnapshot_.load();
    }

    template <typename SampleType>
    void Analyzer<SampleType>::publishSnapshot() noexcept
    {
        Snapshot snapshot {};
        snapshot.prepared = prepared_;
        if (!prepared_)
        {
            atomicSnapshot_.store(snapshot);
            return;
        }

        snapshot.activeChannelCount = config_.numberOfChannels;
        snapshot.sampleRate = config_.sampleRate;
        snapshot.processedSampleCount = processedSampleCount_;
        snapshot.processedBlockCount = processedBlockCount_;
        snapshot.channels[0] = levelTrackers_[0].metrics();
        snapshot.channels[1] = levelTrackers_[1].metrics();
        snapshot.channelEvents[0] = eventTrackers_[0].events();
        snapshot.channelEvents[1] = eventTrackers_[1].events();
        snapshot.stereoEvents = stereoEventTracker_.events();

        DiagnosticFlags flags = static_cast<DiagnosticFlags>(stickyFlags_)
                                | DiagnosticFlags::prepared;
        for (std::size_t channel = 0U; channel < config_.numberOfChannels; ++channel)
        {
            const ChannelMetrics& metrics = snapshot.channels[channel];
            if (metrics.isSilent)
            {
                flags |= DiagnosticFlags::silenceDetected;
            }
            if (metrics.clipCount > 0U)
            {
                flags |= DiagnosticFlags::clippingDetected;
            }
            if (metrics.invalidSampleCount > 0U)
            {
                flags |= DiagnosticFlags::invalidSampleDetected;
            }
        }

        if (config_.numberOfChannels == maximumSupportedChannels)
        {
            const ChannelMetrics& left = snapshot.channels[0];
            const ChannelMetrics& right = snapshot.channels[1];
            snapshot.stereo = makeStereoMetrics(config_, left, right, correlationTracker_);

            if (snapshot.stereo.identicalChannels)
            {
                flags |= DiagnosticFlags::identicalChannelsDetected;
            }
            if (snapshot.stereo.reversedPolarity)
            {
                flags |= DiagnosticFlags::reversedPolarityDetected;
            }
            if (snapshot.stereo.leftOnly)
            {
                flags |= DiagnosticFlags::leftOnlyDetected;
            }
            if (snapshot.stereo.rightOnly)
            {
                flags |= DiagnosticFlags::rightOnlyDetected;
            }
        }

        snapshot.diagnosticFlags = flags;
        atomicSnapshot_.store(snapshot);
    }

    template class Analyzer<float>;
    template class Analyzer<double>;
}
