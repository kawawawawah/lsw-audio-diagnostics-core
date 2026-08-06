// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "finite_value_guard.hpp"

namespace lsw::audio_diag::detail
{
    static_assert(static_cast<int>(SampleClassification::finite) == 0,
                  "Sample classification values are intentionally stable internally.");
}
