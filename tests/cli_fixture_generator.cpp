// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "wav_fixture_builder.hpp"
#include <iostream>

using namespace lsw::audio_diag::test;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: lsw_audio_diagnostics_cli_fixture_generator <type> <output.wav>\n";
        return 1;
    }

    std::string type = argv[1];
    std::string path = argv[2];

    try
    {
        if (type == "valid_mono")
        {
            WavFixtureBuilder()
                .setChannels(1).setSampleRate(48000).setBitsPerSample(16).setValidBitsPerSample(16).setFormatTag(1)
                .addSample(static_cast<std::int16_t>(10000))
                .addSample(static_cast<std::int16_t>(-10000))
                .writeToFile(path);
        }
        else if (type == "valid_stereo")
        {
            WavFixtureBuilder()
                .setChannels(2).setSampleRate(48000).setBitsPerSample(32).setValidBitsPerSample(32).setExtensible(true)
                .addSample(0.5f).addSample(0.5f)
                .addSample(-0.5f).addSample(-0.5f)
                .writeToFile(path);
        }
        else if (type == "partial_4097")
        {
            WavFixtureBuilder builder;
            builder.setChannels(1).setSampleRate(48000).setBitsPerSample(16);
            for (int i = 0; i < 4097; ++i)
            {
                builder.addSample(static_cast<std::int16_t>(100));
            }
            builder.writeToFile(path);
        }
        else if (type == "malformed")
        {
            WavFixtureBuilder()
                .setChannels(1).setSampleRate(48000).setBitsPerSample(16).setValidBitsPerSample(16)
                .omitFmtChunk(true)
                .writeToFile(path);
        }
        else
        {
            std::cerr << "Unknown fixture type: " << type << "\n";
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error writing fixture: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
