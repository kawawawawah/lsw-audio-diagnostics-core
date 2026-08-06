// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "cli_arguments.hpp"
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

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i)
        {
            args.push_back(argv[i]);
        }

        auto parsed = lsw::audio_diag::cli::parseCliArguments(args);
        if (!parsed.success)
        {
            if (parsed.errorMessage.empty())
            {
                lsw::audio_diag::cli::printHelp();
            }
            else
            {
                std::cerr << parsed.errorMessage;
            }
            return parsed.exitCode;
        }

        if (parsed.command == lsw::audio_diag::cli::Command::help)
        {
            lsw::audio_diag::cli::printHelp();
            return 0;
        }

        if (parsed.command == lsw::audio_diag::cli::Command::version)
        {
            std::cout << lsw::audio_diag::versionString << '\n';
            return 0;
        }

        std::string inputPath = parsed.inputPath;
        std::string outputPath = parsed.outputPath;
        bool pretty = parsed.pretty;
        std::size_t blockSize = parsed.blockSize;
        bool hasOutput = !outputPath.empty();

        std::error_code ec;
        if (hasOutput)
        {
            if (inputPath == outputPath)
            {
                std::cerr << "Error: Input and output paths resolve to the same file\n";
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
            auto readRes = reader.readBlock(channels, blockSize);
            if (readRes.status == lsw::audio_diag::offline::WavReadStatus::end_of_stream) break;
            if (readRes.status != lsw::audio_diag::offline::WavReadStatus::success)
            {
                std::cerr << "Error: Read failed\n";
                return 4;
            }

            channelPtrs.resize(channels.size());
            for (std::size_t i = 0; i < channels.size(); ++i)
            {
                channelPtrs[i] = channels[i].data();
            }

            analyzer.process(channelPtrs.data(), channels.size(), readRes.frameCount);
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
            fs::path backupPath = outPath.parent_path() / (outPath.filename().string() + ".bak");
            if (outPath.parent_path().empty()) {
                tempPath = outPath.string() + ".tmp";
                backupPath = outPath.string() + ".bak";
            }

            std::ofstream out(tempPath, std::ios::binary);
            if (!out)
            {
                std::cerr << "Error: Failed to create output file\n";
                return 6;
            }

            out.write(json.data(), json.size());
            out.flush();
            if (!out)
            {
                std::cerr << "Error: Failed to write output file\n";
                out.close();
                fs::remove(tempPath, ec);
                return 6;
            }
            out.close();

            bool backupCreated = false;
            if (fs::exists(outPath, ec))
            {
                fs::remove(backupPath, ec);
                fs::rename(outPath, backupPath, ec);
                if (ec)
                {
                    std::cerr << "Error: Failed to create backup of existing output file\n";
                    fs::remove(tempPath, ec);
                    return 6;
                }
                backupCreated = true;
            }

            fs::rename(tempPath, outPath, ec);
            if (ec)
            {
                std::cerr << "Error: Failed to replace output file\n";
                if (backupCreated)
                {
                    fs::rename(backupPath, outPath, ec);
                }
                fs::remove(tempPath, ec);
                return 6;
            }

            if (backupCreated)
            {
                fs::remove(backupPath, ec);
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
