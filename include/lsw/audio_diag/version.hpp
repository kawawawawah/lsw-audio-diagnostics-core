// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#define LSW_AUDIO_DIAG_VERSION_MAJOR 0
#define LSW_AUDIO_DIAG_VERSION_MINOR 1
#define LSW_AUDIO_DIAG_VERSION_PATCH 0
#define LSW_AUDIO_DIAG_VERSION_STRING "0.1.0"

namespace lsw::audio_diag
{
    inline constexpr int versionMajor = LSW_AUDIO_DIAG_VERSION_MAJOR;
    inline constexpr int versionMinor = LSW_AUDIO_DIAG_VERSION_MINOR;
    inline constexpr int versionPatch = LSW_AUDIO_DIAG_VERSION_PATCH;
    inline constexpr const char* versionString = LSW_AUDIO_DIAG_VERSION_STRING;
}
