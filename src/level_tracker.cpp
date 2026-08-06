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
    }

    void LevelTracker::configure(const AnalyzerConfig& config) noexcept
    {
        levelAlpha_ = std::exp(-1.0 / (config.levelTimeConstantSeconds * config.sampleRate));
        dcAlpha_ = std::exp(-1.0 / (config.dcTimeConstantSeconds * config.sampleRate));
        silenceThresholdDbfs_ = config.silenceThresholdDbfs;
        clipThreshold_ = config.clipThreshold;

        const double requestedSamples = std::ceil(config.silenceHoldSeconds * config.sampleRate);
        const double maximumSamples = static_cast<double>(std::numeric_limits<std::uint64_t>::max());
        silenceHoldSamples_ = requestedSamples >= maximumSamples
                                  ? std::numeric_limits<std::uint64_t>::max()
                                  : static_cast<std::uint64_t>(requestedSamples);
        reset();
    }

    void LevelTracker::reset() noexcept
    {
        smoothedMeanSquare_ = 0.0;
        smoothedDc_ = 0.0;
        blockPeak_ = 0.0;
        maximumAbsoluteSample_ = 0.0;
        clipCount_ = 0U;
        consecutiveClipCount_ = 0U;
        invalidSampleCount_ = 0U;
        nanCount_ = 0U;
        positiveInfinityCount_ = 0U;
        negativeInfinityCount_ = 0U;
        denormalCount_ = 0U;
        silenceSamples_ = 0U;
        isSilent_ = false;
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
                ++invalidSampleCount_;
                ++nanCount_;
                break;
            case SampleClassification::positiveInfinity:
                ++invalidSampleCount_;
                ++positiveInfinityCount_;
                break;
            case SampleClassification::negativeInfinity:
                ++invalidSampleCount_;
                ++negativeInfinityCount_;
                break;
            case SampleClassification::denormal:
                ++denormalCount_;
                break;
        }

        const double absoluteSample = std::abs(sample);
        blockPeak_ = std::max(blockPeak_, absoluteSample);
        maximumAbsoluteSample_ = std::max(maximumAbsoluteSample_, absoluteSample);
        smoothedMeanSquare_ = (levelAlpha_ * smoothedMeanSquare_)
                               + ((1.0 - levelAlpha_) * sample * sample);
        smoothedDc_ = (dcAlpha_ * smoothedDc_) + ((1.0 - dcAlpha_) * sample);

        if (absoluteSample >= clipThreshold_)
        {
            ++clipCount_;
            ++consecutiveClipCount_;
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
            const std::uint64_t remaining = std::numeric_limits<std::uint64_t>::max() - silenceSamples_;
            silenceSamples_ += std::min(blockSamples, remaining);
            isSilent_ = silenceSamples_ >= silenceHoldSamples_;
        }
        else
        {
            silenceSamples_ = 0U;
            isSilent_ = false;
        }
    }

    ChannelMetrics LevelTracker::metrics() const noexcept
    {
        const double rms = std::sqrt(std::max(smoothedMeanSquare_, 0.0));
        ChannelMetrics result {};
        result.samplePeak = blockPeak_;
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
