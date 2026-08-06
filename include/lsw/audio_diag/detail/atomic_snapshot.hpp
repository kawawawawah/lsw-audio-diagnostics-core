// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/snapshot.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>

namespace lsw::audio_diag::detail
{
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "LSW Audio Diagnostics Core requires nonblocking 64-bit integer atomics.");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "LSW Audio Diagnostics Core requires nonblocking 32-bit integer atomics.");

    [[nodiscard]] inline std::uint64_t doubleToBits(const double value) noexcept
    {
        std::uint64_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    [[nodiscard]] inline double bitsToDouble(const std::uint64_t bits) noexcept
    {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    class AtomicChannelMetrics
    {
    public:
        void store(const ChannelMetrics& value) noexcept;
        [[nodiscard]] ChannelMetrics load() const noexcept;

    private:
        std::atomic<std::uint64_t> samplePeak_ { 0U };
        std::atomic<std::uint64_t> heldPeak_ { 0U };
        std::atomic<std::uint64_t> heldPeakDbfs_ { 0U };
        std::atomic<std::uint64_t> smoothedRms_ { 0U };
        std::atomic<std::uint64_t> rmsDbfs_ { 0U };
        std::atomic<std::uint64_t> dcOffset_ { 0U };
        std::atomic<std::uint64_t> maximumAbsoluteSample_ { 0U };
        std::atomic<std::uint64_t> clipCount_ { 0U };
        std::atomic<std::uint64_t> consecutiveClipCount_ { 0U };
        std::atomic<std::uint64_t> invalidSampleCount_ { 0U };
        std::atomic<std::uint64_t> nanCount_ { 0U };
        std::atomic<std::uint64_t> positiveInfinityCount_ { 0U };
        std::atomic<std::uint64_t> negativeInfinityCount_ { 0U };
        std::atomic<std::uint64_t> denormalCount_ { 0U };
        std::atomic<std::uint32_t> isSilent_ { 0U };
    };

    class AtomicEventState
    {
    public:
        void store(const EventState& value) noexcept;
        [[nodiscard]] EventState load() const noexcept;

    private:
        std::atomic<std::uint32_t> state_ { 0U };
        std::atomic<std::uint64_t> eventCount_ { 0U };
        std::atomic<std::uint64_t> currentDurationSamples_ { 0U };
        std::atomic<std::uint64_t> longestDurationSamples_ { 0U };
        std::atomic<std::uint64_t> lastStartedAtSample_ { 0U };
    };

    class AtomicChannelEvents
    {
    public:
        void store(const ChannelEvents& value) noexcept;
        [[nodiscard]] ChannelEvents load() const noexcept;

    private:
        AtomicEventState dropout_ {};
        AtomicEventState sustainedClip_ {};
        AtomicEventState dcFault_ {};
        AtomicEventState invalidBurst_ {};
        std::atomic<std::uint64_t> maximumObservedDcOffset_ { 0U };
        std::atomic<std::uint64_t> maximumInvalidSamplesPerBlock_ { 0U };
    };

    class AtomicStereoEvents
    {
    public:
        void store(const StereoEvents& value) noexcept;
        [[nodiscard]] StereoEvents load() const noexcept;

    private:
        AtomicEventState reversedPolarity_ {};
        AtomicEventState identicalChannels_ {};
        AtomicEventState leftOnly_ {};
        AtomicEventState rightOnly_ {};
    };

    class AtomicStereoMetrics
    {
    public:
        void store(const StereoMetrics& value) noexcept;
        [[nodiscard]] StereoMetrics load() const noexcept;

    private:
        std::atomic<std::uint64_t> correlation_ { 0U };
        std::atomic<std::uint64_t> leftRms_ { 0U };
        std::atomic<std::uint64_t> rightRms_ { 0U };
        std::atomic<std::uint64_t> channelBalanceDb_ { 0U };
        std::atomic<std::uint64_t> monoCompatibilityScore_ { 0U };
        std::atomic<std::uint32_t> state_ { 0U };
    };

    /** Single-writer sequence-guarded atomic field snapshot. */
    class AtomicSnapshot
    {
    public:
        void store(const Snapshot& value) noexcept;
        [[nodiscard]] Snapshot load() const noexcept;

    private:
        std::atomic<std::uint64_t> sequence_ { 0U };
        std::atomic<std::uint32_t> prepared_ { 0U };
        std::atomic<std::uint64_t> activeChannelCount_ { 0U };
        std::atomic<std::uint64_t> sampleRate_ { 0U };
        std::atomic<std::uint64_t> processedSampleCount_ { 0U };
        std::atomic<std::uint64_t> processedBlockCount_ { 0U };
        std::atomic<std::uint32_t> diagnosticFlags_ { 0U };
        AtomicChannelMetrics channels_[2] {};
        AtomicStereoMetrics stereo_ {};
        AtomicChannelEvents channelEvents_[2] {};
        AtomicStereoEvents stereoEvents_ {};
    };
}
