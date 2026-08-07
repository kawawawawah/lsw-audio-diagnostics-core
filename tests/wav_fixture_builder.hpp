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

        WavFixtureBuilder& setRiffHeader(const std::string& tag) { riffHeader_ = tag; return *this; }
        WavFixtureBuilder& setWaveHeader(const std::string& tag) { waveHeader_ = tag; return *this; }
        WavFixtureBuilder& setOverrideByteRate(std::uint32_t byteRate) { overrideByteRate_ = byteRate; return *this; }
        WavFixtureBuilder& setOverrideBlockAlign(std::uint16_t blockAlign) { overrideBlockAlign_ = blockAlign; return *this; }
        WavFixtureBuilder& setCustomGuid(const std::uint8_t guid[16]);
        WavFixtureBuilder& setFmtAfterData(bool after) { fmtAfterData_ = after; return *this; }
        WavFixtureBuilder& setDuplicateFmt(bool dup) { duplicateFmt_ = dup; return *this; }
        WavFixtureBuilder& setDuplicateData(bool dup) { duplicateData_ = dup; return *this; }

        WavFixtureBuilder& addSample(std::uint8_t value);
        WavFixtureBuilder& addSample(std::int16_t value);
        WavFixtureBuilder& addSample(std::int32_t value);
        WavFixtureBuilder& addSample24(std::int32_t value);
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
        WavFixtureBuilder& omitRequiredOddPadding(bool omit) { omitRequiredOddPadding_ = omit; return *this; }
        WavFixtureBuilder& declareTruncatedDataChunk(std::uint32_t declaredSize) { truncatedDataDeclaredSize_ = declaredSize; hasTruncatedDataDecl_ = true; return *this; }
        WavFixtureBuilder& appendChunkBeyondRiff(const std::string& id, std::uint32_t oversizeBy) { appendBeyondRiffId_ = id; appendBeyondRiffOversizeBy_ = oversizeBy; hasAppendBeyondRiff_ = true; return *this; }

        std::vector<std::uint8_t> build() const;
        void writeToFile(const std::string& path) const;

    private:
        std::uint16_t channels_ = 1;
        std::uint32_t sampleRate_ = 44100;
        std::uint16_t bitsPerSample_ = 16;
        std::uint16_t validBitsPerSample_ = 16;
        std::uint16_t formatTag_ = 1; // PCM
        bool isExtensible_ = false;

        std::string riffHeader_ = "RIFF";
        std::string waveHeader_ = "WAVE";
        std::uint32_t overrideByteRate_ = 0xFFFFFFFF;
        std::uint32_t overrideBlockAlign_ = 0xFFFFFFFF;
        bool hasCustomGuid_ = false;
        std::uint8_t customGuid_[16] = {};
        bool fmtAfterData_ = false;
        bool duplicateFmt_ = false;
        bool duplicateData_ = false;

        std::vector<std::uint8_t> dataBytes_;
        
        bool breakRiffHeader_ = false;
        bool omitFmt_ = false;
        bool omitData_ = false;
        bool breakChunkHeader_ = false;
        bool oddPadding_ = false;
        bool omitRequiredOddPadding_ = false;
        bool isFloat_ = false;
        std::uint32_t overrideRiffSize_ = 0xFFFFFFFF;
        bool hasTruncatedDataDecl_ = false;
        std::uint32_t truncatedDataDeclaredSize_ = 0;
        bool hasAppendBeyondRiff_ = false;
        std::string appendBeyondRiffId_;
        std::uint32_t appendBeyondRiffOversizeBy_ = 0;
        
        struct ExtraChunk {
            std::string id;
            std::vector<std::uint8_t> data;
        };
        std::vector<ExtraChunk> extraChunks_;
        
        void writeU32LE(std::vector<std::uint8_t>& buf, std::uint32_t val) const;
        void writeU16LE(std::vector<std::uint8_t>& buf, std::uint16_t val) const;
        void writeFmtChunk(std::vector<std::uint8_t>& out) const;
        void writeDataChunk(std::vector<std::uint8_t>& out) const;
    };
}
