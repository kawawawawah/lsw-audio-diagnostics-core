// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/offline/wav_reader.hpp"
#include <cstring>
#include <limits>
#include <filesystem>
#include <algorithm>

namespace lsw::audio_diag::offline
{
    namespace
    {
        std::uint16_t readU16LE(const std::uint8_t* data)
        {
            const std::uint32_t b0 = static_cast<std::uint32_t>(data[0]);
            const std::uint32_t b1 = static_cast<std::uint32_t>(data[1]);
            const std::uint32_t val = b0 | (b1 << 8);
            return static_cast<std::uint16_t>(val);
        }

        std::uint32_t readU32LE(const std::uint8_t* data)
        {
            const std::uint32_t b0 = static_cast<std::uint32_t>(data[0]);
            const std::uint32_t b1 = static_cast<std::uint32_t>(data[1]);
            const std::uint32_t b2 = static_cast<std::uint32_t>(data[2]);
            const std::uint32_t b3 = static_cast<std::uint32_t>(data[3]);
            return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
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

        std::uint64_t chunkHeaderOffset = 12;

        while (chunkHeaderOffset < riffEndOffset)
        {
            std::uint64_t chunkDataOffset = 0;
            if (!checkedAdd(chunkHeaderOffset, 8, chunkDataOffset)) return { WavReaderError::malformed_file, {} };
            if (chunkDataOffset > riffEndOffset) return { WavReaderError::malformed_file, {} };

            if (chunkHeaderOffset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
                return { WavReaderError::malformed_file, {} };

            file_.clear();
            file_.seekg(static_cast<std::streamoff>(chunkHeaderOffset), std::ios::beg);

            std::uint8_t chunkHeader[8];
            if (!file_.read(reinterpret_cast<char*>(chunkHeader), 8))
            {
                return { WavReaderError::malformed_file, {} };
            }

            std::uint32_t chunkSize = readU32LE(chunkHeader + 4);
            std::string chunkId(reinterpret_cast<char*>(chunkHeader), 4);

            std::uint64_t chunkDataEnd = 0;
            if (!checkedAdd(chunkDataOffset, chunkSize, chunkDataEnd)) return { WavReaderError::malformed_file, {} };

            std::uint64_t padding = chunkSize % 2;
            std::uint64_t paddedChunkEnd = 0;
            if (!checkedAdd(chunkDataEnd, padding, paddedChunkEnd)) return { WavReaderError::malformed_file, {} };

            if (paddedChunkEnd > riffEndOffset || paddedChunkEnd > physicalFileSize)
                return { WavReaderError::malformed_file, {} };

            if (chunkId == "fmt ")
            {
                if (hasFmt) return { WavReaderError::malformed_file, {} };
                hasFmt = true;

                if (chunkSize < 16) return { WavReaderError::malformed_file, {} };

                std::uint8_t fmtBuf[40] = {};
                std::size_t bytesToReadFromFmt = static_cast<std::size_t>(std::min<std::uint64_t>(chunkSize, 40));
                if (bytesToReadFromFmt > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
                    return { WavReaderError::malformed_file, {} };

                if (!file_.read(reinterpret_cast<char*>(fmtBuf), static_cast<std::streamsize>(bytesToReadFromFmt)))
                    return { WavReaderError::malformed_file, {} };

                formatTag = readU16LE(fmtBuf);
                channels = readU16LE(fmtBuf + 2);
                sampleRate = readU32LE(fmtBuf + 4);
                byteRate = readU32LE(fmtBuf + 8);
                blockAlign = readU16LE(fmtBuf + 12);
                bitsPerSample = readU16LE(fmtBuf + 14);
                validBitsPerSample = bitsPerSample;

                if (formatTag == FORMAT_EXTENSIBLE)
                {
                    if (chunkSize < 40) return { WavReaderError::malformed_file, {} };
                    std::uint16_t cbSize = readU16LE(fmtBuf + 16);
                    if (cbSize < 22) return { WavReaderError::malformed_file, {} };

                    validBitsPerSample = readU16LE(fmtBuf + 18);
                    const std::uint8_t* guid = fmtBuf + 24;

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

                dataChunkStart_ = chunkDataOffset;
                dataChunkSize_ = chunkSize;
            }

            chunkHeaderOffset = paddedChunkEnd;
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

        if (isFloat_ && validBitsPerSample != bitsPerSample)
            return { WavReaderError::unsupported_format, {} };

        std::uint32_t expectedBlockAlign = static_cast<std::uint32_t>(channels) * static_cast<std::uint32_t>(bitsPerSample / 8);
        if (blockAlign != expectedBlockAlign || blockAlign == 0)
            return { WavReaderError::malformed_file, {} };

        std::uint64_t expectedByteRate64 = 0;
        if (!checkedMultiply(sampleRate, blockAlign, expectedByteRate64))
            return { WavReaderError::malformed_file, {} };

        if (static_cast<std::uint64_t>(byteRate) != expectedByteRate64)
            return { WavReaderError::malformed_file, {} };

        if (dataChunkSize_ % blockAlign != 0)
            return { WavReaderError::malformed_file, {} };

        std::uint64_t frames64 = dataChunkSize_ / blockAlign;
        if (frames64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
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
        metadata_.frameCount = static_cast<std::size_t>(frames64);
        metadata_.durationSeconds = static_cast<double>(metadata_.frameCount) / sampleRate;

        framesRead_ = 0;
        file_.clear();

        if (dataChunkStart_ > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
            return { WavReaderError::malformed_file, {} };

        file_.seekg(static_cast<std::streamoff>(dataChunkStart_), std::ios::beg);
        auto pos = file_.tellg();
        if (pos < 0) return { WavReaderError::read_failed, {} };
        std::uint64_t actualPos = static_cast<std::uint64_t>(pos);
        if (actualPos != dataChunkStart_) return { WavReaderError::read_failed, {} };

        return { WavReaderError::success, metadata_ };
    }

    WavReadBlockResult WavReader::readBlock(std::vector<std::vector<double>>& channels, std::size_t maxFrames)
    {
        if (framesRead_ >= metadata_.frameCount) return { WavReadStatus::end_of_stream, 0 };

        std::size_t framesToRead = std::min<std::size_t>(maxFrames, metadata_.frameCount - framesRead_);
        std::uint64_t bytesToRead64 = 0;
        if (!checkedMultiply(framesToRead, blockAlign_, bytesToRead64))
            return { WavReadStatus::malformed_stream, 0 };

        if (bytesToRead64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            return { WavReadStatus::malformed_stream, 0 };

        if (bytesToRead64 > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
            return { WavReadStatus::malformed_stream, 0 };

        std::size_t bytesToRead = static_cast<std::size_t>(bytesToRead64);
        buffer_.resize(bytesToRead);
        if (!file_.read(reinterpret_cast<char*>(buffer_.data()), static_cast<std::streamsize>(bytesToRead)))
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
        std::uint32_t bits = metadata_.bitsPerSample;
        std::uint32_t validBits = metadata_.validBitsPerSample;
        const std::uint8_t* p = buffer_.data();

        for (std::size_t i = 0; i < frames; ++i)
        {
            for (std::uint32_t ch = 0; ch < chCount; ++ch)
            {
                double sample = 0.0;
                if (isFloat_)
                {
                    if (bits == 32)
                    {
                        float f32;
                        std::memcpy(&f32, p, sizeof(float));
                        sample = static_cast<double>(f32);
                        p += 4;
                    }
                    else if (bits == 64)
                    {
                        double f64;
                        std::memcpy(&f64, p, sizeof(double));
                        sample = f64;
                        p += 8;
                    }
                }
                else
                {
                    if (bits == 8)
                    {
                        std::uint8_t u8 = p[0];
                        if (validBits < 8)
                        {
                            std::uint8_t shift = static_cast<std::uint8_t>(8U - validBits);
                            u8 = static_cast<std::uint8_t>((u8 >> shift) << shift);
                        }
                        sample = (static_cast<double>(u8) - 128.0) / 128.0;
                        p += 1;
                    }
                    else if (bits == 16)
                    {
                        std::uint16_t u16 = readU16LE(p);
                        std::int16_t s16 = static_cast<std::int16_t>(u16);
                        if (validBits < 16)
                        {
                            std::uint16_t shift = static_cast<std::uint16_t>(16U - validBits);
                            s16 = static_cast<std::int16_t>((s16 >> shift) << shift);
                        }
                        sample = static_cast<double>(s16) / 32768.0;
                        p += 2;
                    }
                    else if (bits == 24)
                    {
                        std::uint32_t b0 = static_cast<std::uint32_t>(p[0]);
                        std::uint32_t b1 = static_cast<std::uint32_t>(p[1]);
                        std::uint32_t b2 = static_cast<std::uint32_t>(p[2]);
                        std::uint32_t val32 = b0 | (b1 << 8) | (b2 << 16);
                        if (val32 & 0x00800000U)
                        {
                            val32 |= 0xFF000000U;
                        }
                        std::int32_t s24 = static_cast<std::int32_t>(val32);
                        if (validBits < 24)
                        {
                            std::uint32_t shift = 24U - validBits;
                            s24 = (s24 >> shift) << shift;
                        }
                        sample = static_cast<double>(s24) / 8388608.0;
                        p += 3;
                    }
                    else if (bits == 32)
                    {
                        std::uint32_t u32 = readU32LE(p);
                        std::int32_t s32 = static_cast<std::int32_t>(u32);
                        if (validBits < 32)
                        {
                            std::uint32_t shift = 32U - validBits;
                            s32 = (s32 >> shift) << shift;
                        }
                        sample = static_cast<double>(s32) / 2147483648.0;
                        p += 4;
                    }
                }
                channels[ch][i] = sample;
            }
        }
    }
}
