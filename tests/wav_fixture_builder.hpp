// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace lsw::audio_diag::test
{
    class WavFixtureBuilder
    {
    public:
        WavFixtureBuilder();

        WavFixtureBuilder& setChannels(std::uint16_t channels);
        WavFixtureBuilder& setSampleRate(std::uint32_t sampleRate);
        WavFixtureBuilder& setBitsPerSample(std::uint16_t bitsPerSample);
        WavFixtureBuilder& setValidBitsPerSample(std::uint16_t validBitsPerSample);
        WavFixtureBuilder& setFormatTag(std::uint16_t formatTag);
        WavFixtureBuilder& setExtensible(bool isExtensible);
        WavFixtureBuilder& setFloat(bool isFloat) { isFloat_ = isFloat; return *this; }

        WavFixtureBuilder& addSample(std::uint8_t value);
        WavFixtureBuilder& addSample(std::int16_t value);
        WavFixtureBuilder& addSample(std::int32_t value);
        WavFixtureBuilder& addSample(float value);
        WavFixtureBuilder& addSample(double value);
        
        WavFixtureBuilder& addRawBytes(const std::vector<std::uint8_t>& bytes);

        // Intentionally create broken fixtures
        WavFixtureBuilder& breakRiffHeader(bool breakIt);
        WavFixtureBuilder& setDeclaredRiffSize(std::uint32_t size);
        WavFixtureBuilder& omitFmtChunk(bool omit);
        WavFixtureBuilder& omitDataChunk(bool omit);
        WavFixtureBuilder& addUnknownChunk(const std::string& id, const std::vector<std::uint8_t>& data);
        WavFixtureBuilder& breakChunkHeader(bool breakIt);
        WavFixtureBuilder& setOddPadding(bool padding);

        std::vector<std::uint8_t> build() const;
        void writeToFile(const std::string& path) const;

    private:
        std::uint16_t channels_ = 1;
        std::uint32_t sampleRate_ = 44100;
        std::uint16_t bitsPerSample_ = 16;
        std::uint16_t validBitsPerSample_ = 16;
        std::uint16_t formatTag_ = 1; // PCM
        bool isExtensible_ = false;

        std::vector<std::uint8_t> dataBytes_;
        
        bool breakRiffHeader_ = false;
        bool omitFmt_ = false;
        bool omitData_ = false;
        bool breakChunkHeader_ = false;
        bool oddPadding_ = false;
        bool isFloat_ = false;
        std::uint32_t overrideRiffSize_ = 0xFFFFFFFF;
        
        struct ExtraChunk {
            std::string id;
            std::vector<std::uint8_t> data;
        };
        std::vector<ExtraChunk> extraChunks_;
        
        void writeU32LE(std::vector<std::uint8_t>& buf, std::uint32_t val) const;
        void writeU16LE(std::vector<std::uint8_t>& buf, std::uint16_t val) const;
    };
}
