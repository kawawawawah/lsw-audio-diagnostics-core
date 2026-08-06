// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/offline/wav_reader.hpp"
#include <cstring>
#include <limits>
#include <filesystem>

namespace lsw::audio_diag::offline
{
    namespace
    {
        std::uint16_t readU16LE(const std::uint8_t* data)
        {
            return static_cast<std::uint16_t>(data[0]) |
                   (static_cast<std::uint16_t>(data[1]) << 8);
        }

        std::uint32_t readU32LE(const std::uint8_t* data)
        {
            return static_cast<std::uint32_t>(data[0]) |
                   (static_cast<std::uint32_t>(data[1]) << 8) |
                   (static_cast<std::uint32_t>(data[2]) << 16) |
                   (static_cast<std::uint32_t>(data[3]) << 24);
        }

        constexpr std::uint16_t FORMAT_PCM = 0x0001;
        constexpr std::uint16_t FORMAT_IEEE_FLOAT = 0x0003;
        constexpr std::uint16_t FORMAT_EXTENSIBLE = 0xFFFE;

        // GUIDs
        const std::uint8_t GUID_PCM[16] = {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
        };
        const std::uint8_t GUID_IEEE_FLOAT[16] = {
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
        };

        bool checkedAdd(std::uint64_t a, std::uint64_t b, std::uint64_t& out)
        {
            if (std::numeric_limits<std::uint64_t>::max() - a < b) return false;
            out = a + b;
            return true;
        }

        bool checkedMultiply(std::uint64_t a, std::uint64_t b, std::uint64_t& out)
        {
            if (a == 0 || b == 0) { out = 0; return true; }
            if (std::numeric_limits<std::uint64_t>::max() / a < b) return false;
            out = a * b;
            return true;
        }
    }

    WavReader::WavReader() = default;

    WavReader::~WavReader() = default;

    WavReaderResult WavReader::open(const std::string& path)
    {
        std::error_code ec;
        std::uint64_t physicalFileSize = 0;
        if (std::filesystem::exists(path, ec))
        {
            physicalFileSize = std::filesystem::file_size(path, ec);
            if (ec) return { WavReaderError::open_failed, {} };
        }
        else
        {
            return { WavReaderError::open_failed, {} };
        }

        if (physicalFileSize < 12) return { WavReaderError::malformed_file, {} };

        file_.open(path, std::ios::binary);
        if (!file_)
        {
            return { WavReaderError::open_failed, {} };
        }

        std::uint8_t header[12];
        if (!file_.read(reinterpret_cast<char*>(header), 12))
            return { WavReaderError::malformed_file, {} };

        if (std::memcmp(header, "RIFF", 4) != 0 || std::memcmp(header + 8, "WAVE", 4) != 0)
            return { WavReaderError::malformed_file, {} };

        std::uint64_t riffDeclaredSize = readU32LE(header + 4);
        std::uint64_t riffEndOffset = 0;
        if (!checkedAdd(riffDeclaredSize, 8, riffEndOffset)) return { WavReaderError::malformed_file, {} };
        if (riffEndOffset > physicalFileSize) return { WavReaderError::malformed_file, {} };

        bool hasFmt = false;
        bool hasData = false;

        std::uint16_t formatTag = 0;
        std::uint16_t channels = 0;
        std::uint32_t sampleRate = 0;
        std::uint32_t byteRate = 0;
        std::uint16_t blockAlign = 0;
        std::uint16_t bitsPerSample = 0;
        std::uint16_t validBitsPerSample = 0;

        while (true)
        {
            std::uint8_t chunkHeader[8];
            if (!file_.read(reinterpret_cast<char*>(chunkHeader), 8))
            {
                if (file_.eof()) break; // end of file correctly reached maybe, but need data chunk
                return { WavReaderError::read_failed, {} };
            }

            std::uint32_t chunkSize = readU32LE(chunkHeader + 4);
            std::string chunkId(reinterpret_cast<char*>(chunkHeader), 4);

            if (chunkId == "fmt ")
            {
                if (hasFmt) return { WavReaderError::malformed_file, {} };
                hasFmt = true;

                if (chunkSize < 16) return { WavReaderError::malformed_file, {} };

                std::vector<std::uint8_t> fmtData(chunkSize);
                if (!file_.read(reinterpret_cast<char*>(fmtData.data()), chunkSize))
                    return { WavReaderError::malformed_file, {} };

                formatTag = readU16LE(fmtData.data());
                channels = readU16LE(fmtData.data() + 2);
                sampleRate = readU32LE(fmtData.data() + 4);
                byteRate = readU32LE(fmtData.data() + 8);
                blockAlign = readU16LE(fmtData.data() + 12);
                bitsPerSample = readU16LE(fmtData.data() + 14);
                validBitsPerSample = bitsPerSample;

                if (formatTag == FORMAT_EXTENSIBLE)
                {
                    if (chunkSize < 40) return { WavReaderError::malformed_file, {} };
                    std::uint16_t cbSize = readU16LE(fmtData.data() + 16);
                    if (cbSize < 22) return { WavReaderError::malformed_file, {} };

                    validBitsPerSample = readU16LE(fmtData.data() + 18);
                    const std::uint8_t* guid = fmtData.data() + 24;

                    if (std::memcmp(guid, GUID_PCM, 16) == 0)
                    {
                        formatTag = FORMAT_PCM;
                    }
                    else if (std::memcmp(guid, GUID_IEEE_FLOAT, 16) == 0)
                    {
                        formatTag = FORMAT_IEEE_FLOAT;
                    }
                    else
                    {
                        return { WavReaderError::unsupported_format, {} };
                    }
                }
            }
            else if (chunkId == "data")
            {
                if (hasData) return { WavReaderError::malformed_file, {} };
                hasData = true;

                dataChunkStart_ = file_.tellg();
                dataChunkSize_ = chunkSize;

                // skip data for now
                file_.seekg(chunkSize + (chunkSize % 2), std::ios::cur);
                if (file_.fail() && !file_.eof()) {
                    // if it fails to seek past data and it's not EOF, it's an error unless this is the last chunk
                    file_.clear();
                }
            }
            else
            {
                // unknown chunk, skip it
                file_.seekg(chunkSize + (chunkSize % 2), std::ios::cur);
                if (file_.fail() && !file_.eof()) return { WavReaderError::malformed_file, {} };
            }
        }

        if (!hasFmt || !hasData) return { WavReaderError::malformed_file, {} };

        if (channels == 0 || channels > 2) return { WavReaderError::unsupported_format, {} };
        if (sampleRate == 0) return { WavReaderError::malformed_file, {} };

        if (formatTag == FORMAT_PCM)
        {
            if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32)
                return { WavReaderError::unsupported_format, {} };
            isFloat_ = false;
        }
        else if (formatTag == FORMAT_IEEE_FLOAT)
        {
            if (bitsPerSample != 32 && bitsPerSample != 64)
                return { WavReaderError::unsupported_format, {} };
            isFloat_ = true;
        }
        else
        {
            return { WavReaderError::unsupported_format, {} };
        }

        if (validBitsPerSample == 0 || validBitsPerSample > bitsPerSample)
            return { WavReaderError::malformed_file, {} };

        std::uint32_t expectedBlockAlign = channels * (bitsPerSample / 8);
        if (blockAlign != expectedBlockAlign || blockAlign == 0)
            return { WavReaderError::malformed_file, {} };

        if (byteRate != sampleRate * blockAlign)
            return { WavReaderError::malformed_file, {} };

        if (dataChunkSize_ % blockAlign != 0)
            return { WavReaderError::malformed_file, {} };

        blockAlign_ = blockAlign;
        metadata_.container = "RIFF/WAVE";
        
        if (isFloat_) {
            metadata_.encoding = (bitsPerSample == 32) ? "float_f32" : "float_f64";
        } else {
            if (bitsPerSample == 8) metadata_.encoding = "pcm_u8";
            else if (bitsPerSample == 16) metadata_.encoding = "pcm_s16";
            else if (bitsPerSample == 24) metadata_.encoding = "pcm_s24";
            else if (bitsPerSample == 32) metadata_.encoding = "pcm_s32";
        }

        metadata_.sampleRate = sampleRate;
        metadata_.channelCount = channels;
        metadata_.bitsPerSample = bitsPerSample;
        metadata_.validBitsPerSample = validBitsPerSample;
        metadata_.frameCount = dataChunkSize_ / blockAlign;
        metadata_.durationSeconds = static_cast<double>(metadata_.frameCount) / sampleRate;

        framesRead_ = 0;
        file_.clear();
        file_.seekg(dataChunkStart_, std::ios::beg);

        return { WavReaderError::success, metadata_ };
    }

    WavReadBlockResult WavReader::readBlock(std::vector<std::vector<double>>& channels, std::size_t maxFrames)
    {
        if (framesRead_ >= metadata_.frameCount) return { WavReadStatus::end_of_stream, 0 };

        std::size_t framesToRead = std::min<std::size_t>(maxFrames, metadata_.frameCount - framesRead_);
        std::uint64_t bytesToRead64 = 0;
        if (!checkedMultiply(framesToRead, blockAlign_, bytesToRead64))
            return { WavReadStatus::malformed_stream, 0 };

        std::size_t bytesToRead = static_cast<std::size_t>(bytesToRead64);
        buffer_.resize(bytesToRead);
        if (!file_.read(reinterpret_cast<char*>(buffer_.data()), bytesToRead))
            return { WavReadStatus::read_failed, 0 };

        channels.resize(metadata_.channelCount);
        for (auto& ch : channels)
            ch.resize(framesToRead);

        decodeBlock(framesToRead, channels);
        framesRead_ += framesToRead;

        return { WavReadStatus::success, framesToRead };
    }

    void WavReader::decodeBlock(std::size_t frames, std::vector<std::vector<double>>& channels)
    {
        std::uint32_t chCount = metadata_.channelCount;
        std::uint32_t bytesPerSample = metadata_.bitsPerSample / 8;
        const std::uint8_t* p = buffer_.data();

        if (isFloat_)
        {
            if (bytesPerSample == 4)
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        std::uint32_t bits = readU32LE(p);
                        float val;
                        std::memcpy(&val, &bits, 4);
                        channels[c][f] = val;
                        p += 4;
                    }
                }
            }
            else // 8 bytes
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        std::uint32_t lo = readU32LE(p);
                        std::uint32_t hi = readU32LE(p + 4);
                        std::uint64_t bits = static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(hi) << 32);
                        double val;
                        std::memcpy(&val, &bits, 8);
                        channels[c][f] = val;
                        p += 8;
                    }
                }
            }
        }
        else
        {
            if (bytesPerSample == 1)
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        double val = (static_cast<double>(*p) - 128.0) / 128.0;
                        channels[c][f] = val;
                        p += 1;
                    }
                }
            }
            else if (bytesPerSample == 2)
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        std::int16_t val = static_cast<std::int16_t>(readU16LE(p));
                        channels[c][f] = val / 32768.0;
                        p += 2;
                    }
                }
            }
            else if (bytesPerSample == 3)
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        std::uint32_t bits = p[0] | (p[1] << 8) | (p[2] << 16);
                        if (bits & 0x800000) bits |= 0xFF000000;
                        std::int32_t val = static_cast<std::int32_t>(bits);
                        channels[c][f] = val / 8388608.0;
                        p += 3;
                    }
                }
            }
            else // 4 bytes
            {
                for (std::size_t f = 0; f < frames; ++f)
                {
                    for (std::size_t c = 0; c < chCount; ++c)
                    {
                        std::int32_t val = static_cast<std::int32_t>(readU32LE(p));
                        channels[c][f] = val / 2147483648.0;
                        p += 4;
                    }
                }
            }
        }
    }
}
