// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "lsw/audio_diag/offline/wav_reader.hpp"
#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include "lsw/audio_diag/version.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <exception>

namespace fs = std::filesystem;

namespace
{
    void printHelp()
    {
        std::cerr << "Usage: lsw_audio_diagnostics_cli analyze <input.wav> [--output <report.json>] [--pretty] [--block-size <frames>]\n";
        std::cerr << "       lsw_audio_diagnostics_cli --help\n";
        std::cerr << "       lsw_audio_diagnostics_cli --version\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i)
        {
            args.push_back(argv[i]);
        }

        if (args.empty())
        {
            printHelp();
            return 2;
        }

        if (args.size() == 1 && args[0] == "--help")
        {
            printHelp();
            return 0;
        }

        if (args.size() == 1 && args[0] == "--version")
        {
            std::cout << lsw::audio_diag::versionString << '\n';
            return 0;
        }

        if (args[0] != "analyze")
        {
            std::cerr << "Error: First argument must be 'analyze'\n";
            return 2;
        }

        if (args.size() < 2)
        {
            std::cerr << "Error: Missing input file\n";
            return 2;
        }

        std::string inputPath = args[1];
        if (inputPath.empty() || inputPath.front() == '-')
        {
            std::cerr << "Error: Invalid input file\n";
            return 2;
        }

        std::string outputPath;
        bool pretty = false;
        long long blockSize = 4096;
        bool hasOutput = false;
        bool hasPretty = false;
        bool hasBlockSize = false;

        for (std::size_t i = 2; i < args.size(); ++i)
        {
            const auto& arg = args[i];
            if (arg == "--output")
            {
                if (hasOutput) { std::cerr << "Error: Duplicate --output\n"; return 2; }
                if (i + 1 >= args.size()) { std::cerr << "Error: Missing value for --output\n"; return 2; }
                outputPath = args[++i];
                hasOutput = true;
            }
            else if (arg == "--pretty")
            {
                if (hasPretty) { std::cerr << "Error: Duplicate --pretty\n"; return 2; }
                pretty = true;
                hasPretty = true;
            }
            else if (arg == "--block-size")
            {
                if (hasBlockSize) { std::cerr << "Error: Duplicate --block-size\n"; return 2; }
                if (i + 1 >= args.size()) { std::cerr << "Error: Missing value for --block-size\n"; return 2; }
                std::string bsStr = args[++i];
                try {
                    std::size_t pos;
                    blockSize = std::stoll(bsStr, &pos);
                    if (pos != bsStr.length()) throw std::invalid_argument("");
                } catch (...) {
                    std::cerr << "Error: Invalid block size\n";
                    return 2;
                }
                hasBlockSize = true;
            }
            else
            {
                std::cerr << "Error: Unknown or extra argument '" << arg << "'\n";
                return 2;
            }
        }

        if (blockSize < 1 || blockSize > 1048576)
        {
            std::cerr << "Error: Block size out of range\n";
            return 2;
        }

        std::error_code ec;
        if (hasOutput)
        {
            if (inputPath == outputPath)
            {
                std::cerr << "Error: Input and output paths are identical strings\n";
                return 6;
            }
            
            bool isInputExisting = fs::exists(inputPath, ec);
            bool isOutputExisting = fs::exists(outputPath, ec);
            if (isInputExisting && isOutputExisting)
            {
                if (fs::equivalent(inputPath, outputPath, ec))
                {
                    std::cerr << "Error: Input and output paths resolve to the same file\n";
                    return 6;
                }
            }
        }

        lsw::audio_diag::offline::WavReader reader;
        auto result = reader.open(inputPath);
        if (result.error == lsw::audio_diag::offline::WavReaderError::open_failed)
        {
            std::cerr << "Error: Failed to open input file\n";
            return 3;
        }
        if (result.error == lsw::audio_diag::offline::WavReaderError::read_failed)
        {
            std::cerr << "Error: Failed to read input file\n";
            return 3;
        }
        if (result.error == lsw::audio_diag::offline::WavReaderError::malformed_file ||
            result.error == lsw::audio_diag::offline::WavReaderError::unsupported_format)
        {
            std::cerr << "Error: Malformed or unsupported WAV\n";
            return 4;
        }

        std::uint64_t fileSize = 0;
        if (fs::exists(inputPath, ec))
        {
            fileSize = fs::file_size(inputPath, ec);
        }

        lsw::audio_diag::Analyzer<double> analyzer;
        lsw::audio_diag::AnalyzerConfig config;
        config.sampleRate = result.metadata.sampleRate;
        config.maximumBlockSize = blockSize;
        config.numberOfChannels = result.metadata.channelCount;
        
        auto prepareResult = analyzer.prepare(config);
        if (!lsw::audio_diag::isSuccess(prepareResult))
        {
            std::cerr << "Error: Analyzer prepare failed\n";
            return 5;
        }

        std::vector<std::vector<double>> channels;
        std::vector<const double*> channelPtrs;

        while (true)
        {
            std::size_t frames = reader.readBlock(channels, blockSize);
            if (frames == 0) break;

            channelPtrs.resize(channels.size());
            for (std::size_t i = 0; i < channels.size(); ++i)
            {
                channelPtrs[i] = channels[i].data();
            }

            analyzer.process(channelPtrs.data(), channels.size(), frames);
        }

        lsw::audio_diag::offline::ReportModel model;
        model.toolName = "lsw_audio_diagnostics_cli";
        model.toolVersion = lsw::audio_diag::versionString;
        model.input.path = inputPath;
        model.input.fileSizeBytes = fileSize;
        model.audio = reader.getMetadata();
        model.analysis = analyzer.getSnapshot();

        std::string json = lsw::audio_diag::offline::generateJsonReport(model, pretty);

        if (hasOutput)
        {
            fs::path outPath(outputPath);
            fs::path tempPath = outPath.parent_path() / (outPath.filename().string() + ".tmp");
            if (outPath.parent_path().empty()) {
                tempPath = outPath.string() + ".tmp";
            }

            std::ofstream out(tempPath, std::ios::binary);
            if (!out)
            {
                std::cerr << "Error: Failed to create output file\n";
                return 6;
            }

            out.write(json.data(), json.size());
            if (!out)
            {
                std::cerr << "Error: Failed to write output file\n";
                out.close();
                fs::remove(tempPath, ec);
                return 6;
            }
            out.close();

            fs::rename(tempPath, outPath, ec);
            if (ec)
            {
                std::cerr << "Error: Failed to finalize output file\n";
                fs::remove(tempPath, ec); // cleanup best effort
                return 6;
            }
        }
        else
        {
            std::cout << json;
            std::cout.flush();
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Internal error: " << e.what() << '\n';
        return 5;
    }
    catch (...)
    {
        std::cerr << "Internal error\n";
        return 5;
    }
}
