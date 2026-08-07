// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/snapshot.hpp"

#include <string>
#include <cstdint>

namespace lsw::audio_diag::offline
{
    struct AudioMetadata
    {
        std::string container;
        std::string encoding;
        double sampleRate = 0.0;
        std::uint32_t channelCount = 0;
        std::uint32_t bitsPerSample = 0;
        std::uint32_t validBitsPerSample = 0;
        std::uint64_t frameCount = 0;
        double durationSeconds = 0.0;
    };

    struct InputMetadata
    {
        std::string path;
        std::uint64_t fileSizeBytes = 0;
    };

    struct ReportModel
    {
        std::string toolName = "lsw_audio_diagnostics_cli";
        std::string toolVersion = "0.3.0";
        InputMetadata input;
        AudioMetadata audio;
        Snapshot analysis;
    };

    std::string generateJsonReport(const ReportModel& model, bool pretty);
}
