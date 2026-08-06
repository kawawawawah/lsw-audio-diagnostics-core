// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/event_state.hpp"

#include <cstdint>

namespace lsw::audio_diag
{
    /** Event diagnostics accumulated independently for one channel. */
    struct ChannelEvents
    {
        EventState dropout {};
        EventState sustainedClip {};
        EventState dcFault {};
        EventState invalidBurst {};

        double maximumObservedDcOffset = 0.0;
        std::uint64_t maximumInvalidSamplesPerBlock = 0U;
    };

    /** Stereo-only event diagnostics. Values retain neutral defaults for mono input. */
    struct StereoEvents
    {
        EventState reversedPolarity {};
        EventState identicalChannels {};
        EventState leftOnly {};
        EventState rightOnly {};
    };
}
