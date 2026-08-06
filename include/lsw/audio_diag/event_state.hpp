// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace lsw::audio_diag
{
    /** State accumulated for one contiguous diagnostic condition. */
    struct EventState
    {
        bool active = false;
        bool latched = false;

        std::uint64_t eventCount = 0U;
        std::uint64_t currentDurationSamples = 0U;
        std::uint64_t longestDurationSamples = 0U;
        std::uint64_t lastStartedAtSample = 0U;
    };
}
