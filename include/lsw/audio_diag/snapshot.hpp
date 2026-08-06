// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/channel_metrics.hpp"
#include "lsw/audio_diag/diagnostic_flags.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace lsw::audio_diag
{
    /** Stereo-only diagnostics. Values retain neutral defaults for mono input. */
    struct StereoMetrics
    {
        double correlation = 0.0;
        double leftRms = 0.0;
        double rightRms = 0.0;
        double channelBalanceDb = 0.0;
        double monoCompatibilityScore = 0.5;
        bool identicalChannels = false;
        bool reversedPolarity = false;
        bool leftOnly = false;
        bool rightOnly = false;
    };

    /** Immutable-by-value view of an Analyzer's most recently published state. */
    struct Snapshot
    {
        bool prepared = false;
        std::size_t activeChannelCount = 0U;
        double sampleRate = 0.0;
        std::uint64_t processedSampleCount = 0U;
        std::uint64_t processedBlockCount = 0U;
        DiagnosticFlags diagnosticFlags = DiagnosticFlags::none;
        std::array<ChannelMetrics, 2U> channels {};
        StereoMetrics stereo {};
    };
}
