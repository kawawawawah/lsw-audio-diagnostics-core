// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace lsw::audio_diag
{
    /** Diagnostics accumulated for one audio channel. */
    struct ChannelMetrics
    {
        double samplePeak = 0.0;
        double heldPeak = 0.0;
        double heldPeakDbfs = -160.0;
        double smoothedRms = 0.0;
        double rmsDbfs = -160.0;
        double dcOffset = 0.0;
        double maximumAbsoluteSample = 0.0;
        std::uint64_t clipCount = 0U;
        std::uint64_t consecutiveClipCount = 0U;
        std::uint64_t invalidSampleCount = 0U;
        std::uint64_t nanCount = 0U;
        std::uint64_t positiveInfinityCount = 0U;
        std::uint64_t negativeInfinityCount = 0U;
        std::uint64_t denormalCount = 0U;
        bool isSilent = false;
    };
}
