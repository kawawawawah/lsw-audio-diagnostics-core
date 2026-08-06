// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "level_tracker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lsw::audio_diag::detail
{
    namespace
    {
        constexpr double minimumDbfs = -160.0;

        [[nodiscard]] double dbfsFromRms(const double rms) noexcept
        {
            constexpr double epsilon = 1.0e-8;
            const double dbfs = 20.0 * std::log10(std::max(rms, epsilon));
            return std::max(dbfs, minimumDbfs);
        }

        [[nodiscard]] double dbfsFromPeak(const double peak) noexcept
        {
            constexpr double epsilon = 1.0e-8;
            const double dbfs = 20.0 * std::log10(std::max(peak, epsilon));
            return std::max(dbfs, minimumDbfs);
        }

        [[nodiscard]] std::uint64_t addSaturated(const std::uint64_t current,
                                                  const std::uint64_t addition) noexcept
        {
            const std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
            return addition > (maximum - current) ? maximum : current + addition;
        }

        [[nodiscard]] std::uint64_t secondsToSamples(const double seconds,
                                                      const double sampleRate) noexcept
        {
            const double requestedSamples = std::ceil(seconds * sampleRate);
            const double maximumSamples = static_cast<double>(std::numeric_limits<std::uint64_t>::max());
            return requestedSamples >= maximumSamples ? std::numeric_limits<std::uint64_t>::max()
                                                      : static_cast<std::uint64_t>(requestedSamples);
        }

        [[nodiscard]] double finiteSquare(const double value) noexcept
        {
            const double maximum = std::numeric_limits<double>::max();
            const double squareRootMaximum = std::sqrt(maximum);
            return std::abs(value) >= squareRootMaximum ? maximum : value * value;
        }
    }

    void LevelTracker::configure(const AnalyzerConfig& config) noexcept
    {
        levelAlpha_ = std::exp(-1.0 / (config.levelTimeConstantSeconds * config.sampleRate));
        dcAlpha_ = std::exp(-1.0 / (config.dcTimeConstantSeconds * config.sampleRate));
        sampleRate_ = config.sampleRate;
        silenceThresholdDbfs_ = config.silenceThresholdDbfs;
        clipThreshold_ = config.clipThreshold;

        silenceHoldSamples_ = secondsToSamples(config.silenceHoldSeconds, config.sampleRate);
        peakHoldSamples_ = secondsToSamples(config.peakHoldSeconds, config.sampleRate);
        peakHoldDecayDbPerSecond_ = config.peakHoldDecayDbPerSecond;
        reset();
    }

    void LevelTracker::reset() noexcept
    {
        resetLevels();
        resetCounters();
    }

    void LevelTracker::resetLevels() noexcept
    {
        smoothedMeanSquare_ = 0.0;
        smoothedDc_ = 0.0;
        blockPeak_ = 0.0;
        heldPeak_ = 0.0;
        heldPeakRemainingSamples_ = 0U;
        maximumAbsoluteSample_ = 0.0;
        silenceSamples_ = 0U;
        isSilent_ = false;
    }

    void LevelTracker::resetCounters() noexcept
    {
        clipCount_ = 0U;
        consecutiveClipCount_ = 0U;
        invalidSampleCount_ = 0U;
        nanCount_ = 0U;
        positiveInfinityCount_ = 0U;
        negativeInfinityCount_ = 0U;
        denormalCount_ = 0U;
    }

    void LevelTracker::beginBlock() noexcept
    {
        blockPeak_ = 0.0;
    }

    void LevelTracker::processSample(const double sample,
                                     const SampleClassification classification) noexcept
    {
        switch (classification)
        {
            case SampleClassification::finite:
                break;
            case SampleClassification::nan:
                invalidSampleCount_ = addSaturated(invalidSampleCount_, 1U);
                nanCount_ = addSaturated(nanCount_, 1U);
                break;
            case SampleClassification::positiveInfinity:
                invalidSampleCount_ = addSaturated(invalidSampleCount_, 1U);
                positiveInfinityCount_ = addSaturated(positiveInfinityCount_, 1U);
                break;
            case SampleClassification::negativeInfinity:
                invalidSampleCount_ = addSaturated(invalidSampleCount_, 1U);
                negativeInfinityCount_ = addSaturated(negativeInfinityCount_, 1U);
                break;
            case SampleClassification::denormal:
                denormalCount_ = addSaturated(denormalCount_, 1U);
                break;
        }

        const double absoluteSample = std::abs(sample);
        blockPeak_ = std::max(blockPeak_, absoluteSample);
        maximumAbsoluteSample_ = std::max(maximumAbsoluteSample_, absoluteSample);
        smoothedMeanSquare_ = (levelAlpha_ * smoothedMeanSquare_)
                               + ((1.0 - levelAlpha_) * finiteSquare(sample));
        smoothedDc_ = (dcAlpha_ * smoothedDc_) + ((1.0 - dcAlpha_) * sample);

        if (absoluteSample >= clipThreshold_)
        {
            clipCount_ = addSaturated(clipCount_, 1U);
            consecutiveClipCount_ = addSaturated(consecutiveClipCount_, 1U);
        }
        else
        {
            consecutiveClipCount_ = 0U;
        }
    }

    void LevelTracker::endBlock(const std::size_t numberOfSamples) noexcept
    {
        const double rms = std::sqrt(std::max(smoothedMeanSquare_, 0.0));
        if (dbfsFromRms(rms) < silenceThresholdDbfs_)
        {
            const std::uint64_t blockSamples = static_cast<std::uint64_t>(numberOfSamples);
            silenceSamples_ = addSaturated(silenceSamples_, blockSamples);
            isSilent_ = silenceSamples_ >= silenceHoldSamples_;
        }
        else
        {
            silenceSamples_ = 0U;
            isSilent_ = false;
        }

        const std::uint64_t blockSamples = static_cast<std::uint64_t>(numberOfSamples);
        if (blockPeak_ >= heldPeak_)
        {
            heldPeak_ = blockPeak_;
            heldPeakRemainingSamples_ = peakHoldSamples_;
        }
        else if (heldPeak_ > 0.0)
        {
            if (blockSamples < heldPeakRemainingSamples_)
            {
                heldPeakRemainingSamples_ -= blockSamples;
            }
            else
            {
                const std::uint64_t decaySamples = blockSamples - heldPeakRemainingSamples_;
                heldPeakRemainingSamples_ = 0U;
                if (decaySamples != 0U)
                {
                    const double decayDb = peakHoldDecayDbPerSecond_
                                           * (static_cast<double>(decaySamples) / sampleRate_);
                    const double decayFactor = std::pow(10.0, -decayDb / 20.0);
                    const double decayedPeak = heldPeak_ * decayFactor;
                    heldPeak_ = std::isfinite(decayedPeak) && decayedPeak > 1.0e-8
                                    ? decayedPeak
                                    : 0.0;
                }
                if (heldPeak_ <= blockPeak_)
                {
                    heldPeak_ = blockPeak_;
                    heldPeakRemainingSamples_ = peakHoldSamples_;
                }
            }
        }
    }

    ChannelMetrics LevelTracker::metrics() const noexcept
    {
        const double rms = std::sqrt(std::max(smoothedMeanSquare_, 0.0));
        ChannelMetrics result {};
        result.samplePeak = blockPeak_;
        result.heldPeak = heldPeak_;
        result.heldPeakDbfs = dbfsFromPeak(heldPeak_);
        result.smoothedRms = rms;
        result.rmsDbfs = dbfsFromRms(rms);
        result.dcOffset = smoothedDc_;
        result.maximumAbsoluteSample = maximumAbsoluteSample_;
        result.clipCount = clipCount_;
        result.consecutiveClipCount = consecutiveClipCount_;
        result.invalidSampleCount = invalidSampleCount_;
        result.nanCount = nanCount_;
        result.positiveInfinityCount = positiveInfinityCount_;
        result.negativeInfinityCount = negativeInfinityCount_;
        result.denormalCount = denormalCount_;
        result.isSilent = isSilent_;
        return result;
    }
}
