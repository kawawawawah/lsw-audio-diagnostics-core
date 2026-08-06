// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace lsw::audio_diag
{
    /** Bit flags describing the most recent diagnostic state and sticky input faults. */
    enum class DiagnosticFlags : std::uint32_t
    {
        none = 0U,
        prepared = 1U << 0U,
        silenceDetected = 1U << 1U,
        clippingDetected = 1U << 2U,
        invalidSampleDetected = 1U << 3U,
        identicalChannelsDetected = 1U << 4U,
        reversedPolarityDetected = 1U << 5U,
        leftOnlyDetected = 1U << 6U,
        rightOnlyDetected = 1U << 7U,
        nullInput = 1U << 8U,
        channelCountMismatch = 1U << 9U,
        blockSizeExceeded = 1U << 10U
    };

    [[nodiscard]] constexpr DiagnosticFlags operator|(const DiagnosticFlags left,
                                                        const DiagnosticFlags right) noexcept
    {
        return static_cast<DiagnosticFlags>(static_cast<std::uint32_t>(left)
                                            | static_cast<std::uint32_t>(right));
    }

    constexpr DiagnosticFlags& operator|=(DiagnosticFlags& left,
                                           const DiagnosticFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    [[nodiscard]] constexpr bool hasFlag(const DiagnosticFlags flags,
                                         const DiagnosticFlags flag) noexcept
    {
        return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0U;
    }
}
