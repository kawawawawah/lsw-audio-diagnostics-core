// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "wav_fixture_builder.hpp"
#include "lsw/audio_diag/offline/wav_reader.hpp"
#include <filesystem>
#include <cmath>
#include <limits>

namespace lsw::audio_diag::test
{
    using namespace lsw::audio_diag::offline;
    namespace fs = std::filesystem;

    struct TestFileGuard
    {
        std::string path;
        TestFileGuard(std::string p) : path(std::move(p)) {}
        ~TestFileGuard() {
            std::error_code ec;
            fs::remove(path, ec);
        }
    };

    LSW_TEST_CASE(WavReader_Valid_MonoStereo_PcmFormatMatrix)
    {
        // PCM8
        {
            TestFileGuard guard("test_pcm8.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(8).addSample(static_cast<std::uint8_t>(255));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));
            LSW_CHECK_EQ(res.metadata.channelCount, 1U);
            LSW_CHECK_EQ(res.metadata.bitsPerSample, 8U);

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK_EQ(readRes.frameCount, 1U);
            LSW_CHECK(ch[0][0] > 0.9);
        }

        // PCM16 Stereo
        {
            TestFileGuard guard("test_pcm16.wav");
            WavFixtureBuilder builder;
            builder.setChannels(2).setBitsPerSample(16).addSample(static_cast<std::int16_t>(32767)).addSample(static_cast<std::int16_t>(-32768));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));
            LSW_CHECK_EQ(res.metadata.channelCount, 2U);

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK(ch[0][0] > 0.99);
            LSW_CHECK(ch[1][0] < -0.99);
        }

        // PCM24
        {
            TestFileGuard guard("test_pcm24.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(24).addSample24(static_cast<std::int32_t>(8388607));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK(ch[0][0] > 0.99);
        }

        // PCM32
        {
            TestFileGuard guard("test_pcm32.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(32).addSample(static_cast<std::int32_t>(2147483647));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK(ch[0][0] > 0.99);
        }

        // Float32
        {
            TestFileGuard guard("test_float32.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(32).setFloat(true).setFormatTag(3).addSample(0.5f);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK_EQ(ch[0][0], 0.5);
        }

        // Float64
        {
            TestFileGuard guard("test_float64.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(64).setFloat(true).setFormatTag(3).addSample(-0.75);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK_EQ(ch[0][0], -0.75);
        }

        // Extensible PCM
        {
            TestFileGuard guard("test_ext_pcm.wav");
            WavFixtureBuilder builder;
            builder.setChannels(2).setBitsPerSample(16).setExtensible(true).addSample(static_cast<std::int16_t>(16384)).addSample(static_cast<std::int16_t>(0));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK(ch[0][0] > 0.49 && ch[0][0] < 0.51);
        }

        // Extensible Float
        {
            TestFileGuard guard("test_ext_float.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(32).setFloat(true).setExtensible(true).addSample(0.25f);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            auto readRes = reader.readBlock(ch, 10);
            LSW_CHECK_EQ(static_cast<int>(readRes.status), static_cast<int>(WavReadStatus::success));
            LSW_CHECK_EQ(ch[0][0], 0.25);
        }
    }

    LSW_TEST_CASE(WavReader_Valid_ChunksAndPadding)
    {
        // Unknown chunk & Odd padding
        {
            TestFileGuard guard("test_unknown_chunk.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(8).addSample(static_cast<std::uint8_t>(128));
            builder.addUnknownChunk("junk", {1, 2, 3}); // odd size unknown chunk
            builder.setOddPadding(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));
        }

        // Data before Fmt
        {
            TestFileGuard guard("test_data_before_fmt.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(16).addSample(static_cast<std::int16_t>(0));
            builder.setFmtAfterData(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));
        }

        // Partial Block Reading
        {
            TestFileGuard guard("test_partial_block.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(16);
            for (int i = 0; i < 5; ++i) builder.addSample(static_cast<std::int16_t>(i));
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));
            LSW_CHECK_EQ(res.metadata.frameCount, 5U);

            std::vector<std::vector<double>> ch;
            auto r1 = reader.readBlock(ch, 2);
            LSW_CHECK_EQ(r1.frameCount, 2U);
            auto r2 = reader.readBlock(ch, 4);
            LSW_CHECK_EQ(r2.frameCount, 3U);
            auto r3 = reader.readBlock(ch, 4);
            LSW_CHECK_EQ(static_cast<int>(r3.status), static_cast<int>(WavReadStatus::end_of_stream));
        }
    }

    LSW_TEST_CASE(WavReader_Conversion_Values)
    {
        // Special float values (NaN, +Inf, -Inf, Float > 1.0)
        {
            TestFileGuard guard("test_conversion_float.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(32).setFloat(true).setFormatTag(3);
            builder.addSample(std::numeric_limits<float>::quiet_NaN());
            builder.addSample(std::numeric_limits<float>::infinity());
            builder.addSample(-std::numeric_limits<float>::infinity());
            builder.addSample(2.5f);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            reader.readBlock(ch, 10);
            LSW_CHECK(std::isnan(ch[0][0]));
            LSW_CHECK(std::isinf(ch[0][1]) && ch[0][1] > 0);
            LSW_CHECK(std::isinf(ch[0][2]) && ch[0][2] < 0);
            LSW_CHECK_EQ(ch[0][3], 2.5);
        }

        // PCM24 Sign Extension
        {
            TestFileGuard guard("test_pcm24_sign.wav");
            WavFixtureBuilder builder;
            builder.setChannels(1).setBitsPerSample(24);
            builder.addSample24(static_cast<std::int32_t>(-8388608)); // -1.0
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::success));

            std::vector<std::vector<double>> ch;
            reader.readBlock(ch, 10);
            LSW_CHECK_EQ(ch[0][0], -1.0);
        }
    }

    LSW_TEST_CASE(WavReader_Invalid_FormatsAndErrors)
    {
        // Non-existent file
        {
            WavReader reader;
            auto res = reader.open("non_existent_file_xyz.wav");
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::open_failed));
        }

        // Truncated RIFF
        {
            TestFileGuard guard("test_inv_trunc_riff.wav");
            WavFixtureBuilder builder;
            builder.breakRiffHeader(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // RF64 / RIFX
        {
            TestFileGuard guard("test_inv_rf64.wav");
            WavFixtureBuilder builder;
            builder.setRiffHeader("RF64");
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Compressed Codec (FormatTag 2 = ADPCM)
        {
            TestFileGuard guard("test_inv_adpcm.wav");
            WavFixtureBuilder builder;
            builder.setFormatTag(2);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::unsupported_format));
        }

        // Channels 0 / Channels 3
        {
            TestFileGuard guard("test_inv_ch3.wav");
            WavFixtureBuilder builder;
            builder.setChannels(3);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::unsupported_format));
        }

        // SampleRate 0
        {
            TestFileGuard guard("test_inv_sr0.wav");
            WavFixtureBuilder builder;
            builder.setSampleRate(0);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Missing Fmt
        {
            TestFileGuard guard("test_inv_no_fmt.wav");
            WavFixtureBuilder builder;
            builder.omitFmtChunk(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Missing Data
        {
            TestFileGuard guard("test_inv_no_data.wav");
            WavFixtureBuilder builder;
            builder.omitDataChunk(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Duplicate Fmt
        {
            TestFileGuard guard("test_inv_dup_fmt.wav");
            WavFixtureBuilder builder;
            builder.setDuplicateFmt(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Duplicate Data
        {
            TestFileGuard guard("test_inv_dup_data.wav");
            WavFixtureBuilder builder;
            builder.setDuplicateData(true);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }

        // Invalid GUID
        {
            TestFileGuard guard("test_inv_guid.wav");
            WavFixtureBuilder builder;
            builder.setExtensible(true);
            std::uint8_t badGuid[16] = {0x99};
            builder.setCustomGuid(badGuid);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::unsupported_format));
        }

        // Invalid BlockAlign / ByteRate
        {
            TestFileGuard guard("test_inv_align.wav");
            WavFixtureBuilder builder;
            builder.setOverrideBlockAlign(99);
            builder.writeToFile(guard.path);

            WavReader reader;
            auto res = reader.open(guard.path);
            LSW_CHECK_EQ(static_cast<int>(res.error), static_cast<int>(WavReaderError::malformed_file));
        }
    }
}
