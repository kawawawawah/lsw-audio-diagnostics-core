// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/analyzer_config.hpp"
#include "lsw/audio_diag/audio_block_view.hpp"
#include "lsw/audio_diag/detail/atomic_snapshot.hpp"
#include "lsw/audio_diag/detail/correlation_tracker.hpp"
#include "lsw/audio_diag/detail/level_tracker.hpp"
#include "lsw/audio_diag/snapshot.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace lsw::audio_diag
{
    /**
     * Fixed-capacity, real-time-safe analyser for mono and stereo float or double audio.
     * Call prepare before process, and call getSnapshot from a monitoring thread as needed.
     */
    template <typename SampleType>
    class Analyzer
    {
        static_assert(std::is_same_v<SampleType, float> || std::is_same_v<SampleType, double>,
                      "Analyzer supports float and double samples only.");

    public:
        Analyzer() noexcept;
        ~Analyzer() = default;

        // Copy construction and assignment are implicitly unavailable because move operations exist.
        Analyzer(Analyzer&& other) noexcept;
        Analyzer& operator=(Analyzer&& other) noexcept;

        /** Validates and installs the configuration; no exception is ever thrown. */
        PrepareResult prepare(const AnalyzerConfig& config) noexcept;
        void reset() noexcept;

        /**
         * Processes a non-owning block without allocation, locks, I/O, logging, or exceptions.
         * Null and mismatched inputs are recorded as diagnostics and handled safely.
         */
        void process(const SampleType* const* channels,
                     std::size_t numberOfChannels,
                     std::size_t numberOfSamples) noexcept;

        void process(const AudioBlockView<SampleType> block) noexcept
        {
            process(block.channels, block.numberOfChannels, block.numberOfSamples);
        }

        [[nodiscard]] Snapshot getSnapshot() const noexcept;

    private:
        void publishSnapshot() noexcept;
        void moveFrom(Analyzer&& other) noexcept;

        AnalyzerConfig config_ {};
        detail::LevelTracker levelTrackers_[2] {};
        detail::CorrelationTracker correlationTracker_ {};
        detail::AtomicSnapshot atomicSnapshot_ {};
        bool prepared_ = false;
        std::uint64_t processedSampleCount_ = 0U;
        std::uint64_t processedBlockCount_ = 0U;
        std::uint32_t stickyFlags_ = 0U;
    };

    extern template class Analyzer<float>;
    extern template class Analyzer<double>;
}
