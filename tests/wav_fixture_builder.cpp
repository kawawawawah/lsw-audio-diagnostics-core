// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "wav_fixture_builder.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>

namespace lsw::audio_diag::test
{
    WavFixtureBuilder::WavFixtureBuilder() = default;

    WavFixtureBuilder& WavFixtureBuilder::setChannels(std::uint16_t channels)
    {
        channels_ = channels;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setSampleRate(std::uint32_t sampleRate)
    {
        sampleRate_ = sampleRate;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setBitsPerSample(std::uint16_t bitsPerSample)
    {
        bitsPerSample_ = bitsPerSample;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setValidBitsPerSample(std::uint16_t validBitsPerSample)
    {
        validBitsPerSample_ = validBitsPerSample;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setFormatTag(std::uint16_t formatTag)
    {
        formatTag_ = formatTag;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setExtensible(bool isExtensible)
    {
        isExtensible_ = isExtensible;
        if (isExtensible_) formatTag_ = 0xFFFE;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addSample(std::uint8_t value)
    {
        dataBytes_.push_back(value);
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addSample(std::int16_t value)
    {
        std::uint16_t u;
        std::memcpy(&u, &value, 2);
        dataBytes_.push_back(static_cast<std::uint8_t>(u & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 8) & 0xFF));
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addSample(std::int32_t value)
    {
        std::uint32_t u;
        std::memcpy(&u, &value, 4);
        dataBytes_.push_back(static_cast<std::uint8_t>(u & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 8) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 16) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 24) & 0xFF));
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addSample(float value)
    {
        std::uint32_t u;
        std::memcpy(&u, &value, 4);
        dataBytes_.push_back(static_cast<std::uint8_t>(u & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 8) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 16) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 24) & 0xFF));
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addSample(double value)
    {
        std::uint64_t u;
        std::memcpy(&u, &value, 8);
        dataBytes_.push_back(static_cast<std::uint8_t>(u & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 8) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 16) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 24) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 32) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 40) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 48) & 0xFF));
        dataBytes_.push_back(static_cast<std::uint8_t>((u >> 56) & 0xFF));
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addRawBytes(const std::vector<std::uint8_t>& bytes)
    {
        dataBytes_.insert(dataBytes_.end(), bytes.begin(), bytes.end());
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::breakRiffHeader(bool breakIt)
    {
        breakRiffHeader_ = breakIt;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setDeclaredRiffSize(std::uint32_t size)
    {
        overrideRiffSize_ = size;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::omitFmtChunk(bool omit)
    {
        omitFmt_ = omit;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::omitDataChunk(bool omit)
    {
        omitData_ = omit;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::addUnknownChunk(const std::string& id, const std::vector<std::uint8_t>& data)
    {
        extraChunks_.push_back({id, data});
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::breakChunkHeader(bool breakIt)
    {
        breakChunkHeader_ = breakIt;
        return *this;
    }

    WavFixtureBuilder& WavFixtureBuilder::setOddPadding(bool padding)
    {
        oddPadding_ = padding;
        return *this;
    }

    void WavFixtureBuilder::writeU32LE(std::vector<std::uint8_t>& buf, std::uint32_t val) const
    {
        buf.push_back(static_cast<std::uint8_t>(val & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((val >> 8) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((val >> 16) & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((val >> 24) & 0xFF));
    }

    void WavFixtureBuilder::writeU16LE(std::vector<std::uint8_t>& buf, std::uint16_t val) const
    {
        buf.push_back(static_cast<std::uint8_t>(val & 0xFF));
        buf.push_back(static_cast<std::uint8_t>((val >> 8) & 0xFF));
    }

    std::vector<std::uint8_t> WavFixtureBuilder::build() const
    {
        std::vector<std::uint8_t> out;

        if (breakRiffHeader_)
        {
            out.insert(out.end(), {'R', 'I', 'F'}); // missing F
            return out;
        }

        out.insert(out.end(), {'R', 'I', 'F', 'F'});
        writeU32LE(out, 0); // Placeholder for size
        out.insert(out.end(), {'W', 'A', 'V', 'E'});

        for (const auto& ch : extraChunks_)
        {
            for (char c : ch.id) out.push_back(c);
            writeU32LE(out, static_cast<std::uint32_t>(ch.data.size()));
            out.insert(out.end(), ch.data.begin(), ch.data.end());
            if (ch.data.size() % 2 != 0) out.push_back(0); // padding
        }

        if (!omitFmt_)
        {
            out.insert(out.end(), {'f', 'm', 't', ' '});
            
            if (breakChunkHeader_) {
                // only write 2 bytes of size
                out.push_back(0);
                out.push_back(0);
                return out; // truncate
            }

            std::uint32_t fmtSize = isExtensible_ ? 40 : 16;
            writeU32LE(out, fmtSize);

            writeU16LE(out, formatTag_);
            writeU16LE(out, channels_);
            writeU32LE(out, sampleRate_);
            std::uint32_t byteRate = sampleRate_ * channels_ * (bitsPerSample_ / 8);
            writeU32LE(out, byteRate);
            writeU16LE(out, channels_ * (bitsPerSample_ / 8));
            writeU16LE(out, bitsPerSample_);

            if (isExtensible_)
            {
                writeU16LE(out, 22); // cbSize
                writeU16LE(out, validBitsPerSample_);
                writeU32LE(out, 0); // channel mask
                
                // GUID
                std::uint8_t guid[16] = {
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
                }; // PCM default
                if (isFloat_) {
                    guid[0] = 0x03; // Float
                }
                out.insert(out.end(), guid, guid + 16);
            }
        }

        if (!omitData_)
        {
            out.insert(out.end(), {'d', 'a', 't', 'a'});
            writeU32LE(out, static_cast<std::uint32_t>(dataBytes_.size()));
            out.insert(out.end(), dataBytes_.begin(), dataBytes_.end());

            if (dataBytes_.size() % 2 != 0 || oddPadding_)
            {
                out.push_back(0); // odd padding
            }
        }

        std::uint32_t totalSize = static_cast<std::uint32_t>(out.size() - 8);
        if (overrideRiffSize_ != 0xFFFFFFFF) totalSize = overrideRiffSize_;

        out[4] = static_cast<std::uint8_t>(totalSize & 0xFF);
        out[5] = static_cast<std::uint8_t>((totalSize >> 8) & 0xFF);
        out[6] = static_cast<std::uint8_t>((totalSize >> 16) & 0xFF);
        out[7] = static_cast<std::uint8_t>((totalSize >> 24) & 0xFF);

        return out;
    }

    void WavFixtureBuilder::writeToFile(const std::string& path) const
    {
        std::vector<std::uint8_t> data = build();
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Failed to create fixture file");
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}
