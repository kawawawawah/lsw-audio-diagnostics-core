// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

namespace lsw::audio_diag::offline
{
    enum class WavReaderError
    {
        success = 0,
        open_failed,
        read_failed,
        malformed_file,
        unsupported_format
    };

    struct WavReaderResult
    {
        WavReaderError error = WavReaderError::success;
        AudioMetadata metadata;
    };

    class WavReader
    {
    public:
        WavReader();
        ~WavReader();

        WavReader(const WavReader&) = delete;
        WavReader& operator=(const WavReader&) = delete;

        WavReaderResult open(const std::string& path);
        
        // Returns number of frames read (0 if EOF or error).
        std::size_t readBlock(std::vector<std::vector<double>>& deinterleavedChannels, std::size_t maxFrames);

        const AudioMetadata& getMetadata() const { return metadata_; }

    private:
        std::ifstream file_;
        AudioMetadata metadata_;
        std::uint64_t dataChunkStart_ = 0;
        std::uint64_t dataChunkSize_ = 0;
        std::uint64_t framesRead_ = 0;
        std::uint32_t blockAlign_ = 0;
        bool isFloat_ = false;
        std::vector<std::uint8_t> buffer_;

        void decodeBlock(std::size_t frames, std::vector<std::vector<double>>& channels);
    };
}
