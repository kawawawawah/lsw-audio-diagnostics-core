// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>

namespace lsw::audio_diag::detail
{
    enum class SampleClassification
    {
        finite,
        nan,
        positiveInfinity,
        negativeInfinity,
        denormal
    };

    struct SanitizedSample
    {
        double value = 0.0;
        SampleClassification classification = SampleClassification::finite;
    };

    template <typename SampleType>
    [[nodiscard]] SanitizedSample sanitizeSample(const SampleType sample) noexcept
    {
        if (std::isnan(sample))
        {
            return { 0.0, SampleClassification::nan };
        }
        if (std::isinf(sample))
        {
            return { 0.0, sample > static_cast<SampleType>(0.0)
                              ? SampleClassification::positiveInfinity
                              : SampleClassification::negativeInfinity };
        }
        if (std::fpclassify(sample) == FP_SUBNORMAL)
        {
            return { 0.0, SampleClassification::denormal };
        }
        return { static_cast<double>(sample), SampleClassification::finite };
    }
}
