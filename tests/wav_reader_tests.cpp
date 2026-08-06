// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "lsw/audio_diag/offline/wav_reader.hpp"
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

namespace lsw::audio_diag::test
{
    namespace
    {
        void writeU32LE(std::vector<std::uint8_t>& buf, std::uint32_t v)
        {
            buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        }

        void writeU16LE(std::vector<std::uint8_t>& buf, std::uint16_t v)
        {
            buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        }
        
        std::string createTempWav(const std::vector<std::uint8_t>& data, const std::string& name)
        {
            auto path = fs::temp_directory_path() / name;
            std::ofstream out(path, std::ios::binary);
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
            return path.string();
        }

        std::vector<std::uint8_t> generateValidPcm16Wav()
        {
            std::vector<std::uint8_t> buf;
            // RIFF
            buf.insert(buf.end(), {'R','I','F','F'});
            writeU32LE(buf, 36 + 4); // size
            buf.insert(buf.end(), {'W','A','V','E'});
            // fmt
            buf.insert(buf.end(), {'f','m','t',' '});
            writeU32LE(buf, 16); // chunk size
            writeU16LE(buf, 1); // PCM
            writeU16LE(buf, 2); // stereo
            writeU32LE(buf, 48000); // sample rate
            writeU32LE(buf, 48000 * 2 * 2); // byte rate
            writeU16LE(buf, 4); // block align
            writeU16LE(buf, 16); // bits per sample
            // data
            buf.insert(buf.end(), {'d','a','t','a'});
            writeU32LE(buf, 4); // 4 bytes data = 1 frame
            writeU16LE(buf, 0); // L
            writeU16LE(buf, 0); // R
            return buf;
        }
    }

    LSW_TEST_CASE(WavReader_ValidPcm16)
    {
        auto wavData = generateValidPcm16Wav();
        std::string path = createTempWav(wavData, "valid_pcm16.wav");

        {
            lsw::audio_diag::offline::WavReader reader;
            auto result = reader.open(path);
            
            LSW_CHECK_EQ(static_cast<int>(result.error), static_cast<int>(lsw::audio_diag::offline::WavReaderError::success));
            LSW_CHECK_EQ(result.metadata.channelCount, 2U);
            LSW_CHECK_EQ(result.metadata.sampleRate, 48000.0);
            LSW_CHECK_EQ(result.metadata.frameCount, 1U);
            
            std::vector<std::vector<double>> channels;
            auto frames = reader.readBlock(channels, 1024);
            LSW_CHECK_EQ(frames, 1U);
            LSW_CHECK_EQ(channels.size(), 2U);
            LSW_CHECK_EQ(channels[0].size(), 1U);
        }

        std::error_code ec;
        fs::remove(path, ec);
    }
}
