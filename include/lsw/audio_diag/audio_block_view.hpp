// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>

namespace lsw::audio_diag
{
    /** Non-owning read-only audio block passed to Analyzer::process. */
    template <typename SampleType>
    struct AudioBlockView
    {
        const SampleType* const* channels = nullptr;
        std::size_t numberOfChannels = 0U;
        std::size_t numberOfSamples = 0U;
    };
}
