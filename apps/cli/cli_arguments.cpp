// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "cli_arguments.hpp"
#include <iostream>

namespace lsw::audio_diag::cli
{
    void printHelp()
    {
        std::cout << "Usage: lsw_audio_diagnostics_cli analyze <input.wav> [--output <report.json>] [--pretty] [--block-size <frames>]\n";
        std::cout << "       lsw_audio_diagnostics_cli --help\n";
        std::cout << "       lsw_audio_diagnostics_cli --version\n";
    }

    ParseResult parseCliArguments(const std::vector<std::string>& args)
    {
        if (args.empty())
        {
            return ParseResult::makeError(2, "");
        }

        if (args[0] == "--help")
        {
            if (args.size() > 1) return ParseResult::makeError(2, "Error: Extra argument after --help\n");
            return ParseResult::makeSuccess(Command::help);
        }

        if (args[0] == "--version")
        {
            if (args.size() > 1) return ParseResult::makeError(2, "Error: Extra argument after --version\n");
            return ParseResult::makeSuccess(Command::version);
        }

        if (args[0] != "analyze")
        {
            return ParseResult::makeError(2, "Error: First argument must be 'analyze'\n");
        }

        if (args.size() < 2)
        {
            return ParseResult::makeError(2, "Error: Missing input file\n");
        }

        std::string inputPath = args[1];
        if (inputPath.empty() || inputPath.front() == '-')
        {
            return ParseResult::makeError(2, "Error: Invalid input file\n");
        }

        ParseResult res = ParseResult::makeSuccess(Command::analyze);
        res.inputPath = inputPath;

        bool hasOutput = false;
        bool hasPretty = false;
        bool hasBlockSize = false;

        for (std::size_t i = 2; i < args.size(); ++i)
        {
            const auto& arg = args[i];
            if (arg == "--output")
            {
                if (hasOutput) return ParseResult::makeError(2, "Error: Duplicate --output\n");
                if (i + 1 >= args.size()) return ParseResult::makeError(2, "Error: Missing value for --output\n");
                
                std::string val = args[++i];
                if (val.empty() || val.front() == '-') return ParseResult::makeError(2, "Error: Invalid value for --output\n");
                
                res.outputPath = val;
                hasOutput = true;
            }
            else if (arg == "--pretty")
            {
                if (hasPretty) return ParseResult::makeError(2, "Error: Duplicate --pretty\n");
                res.pretty = true;
                hasPretty = true;
            }
            else if (arg == "--block-size")
            {
                if (hasBlockSize) return ParseResult::makeError(2, "Error: Duplicate --block-size\n");
                if (i + 1 >= args.size()) return ParseResult::makeError(2, "Error: Missing value for --block-size\n");
                
                std::string bsStr = args[++i];
                try {
                    std::size_t pos;
                    long long bs = std::stoll(bsStr, &pos);
                    if (pos != bsStr.length()) return ParseResult::makeError(2, "Error: Invalid block size\n");
                    if (bs < 1 || bs > 1048576) return ParseResult::makeError(2, "Error: Block size out of range\n");
                    res.blockSize = static_cast<std::size_t>(bs);
                } catch (...) {
                    return ParseResult::makeError(2, "Error: Invalid block size\n");
                }
                hasBlockSize = true;
            }
            else
            {
                return ParseResult::makeError(2, "Error: Unknown or extra argument '" + arg + "'\n");
            }
        }

        return res;
    }

    int mapWavReadStatusToExitCode(lsw::audio_diag::offline::WavReadStatus status)
    {
        switch (status)
        {
        case lsw::audio_diag::offline::WavReadStatus::success:
        case lsw::audio_diag::offline::WavReadStatus::end_of_stream:
            return 0;
        case lsw::audio_diag::offline::WavReadStatus::read_failed:
            return 3;
        case lsw::audio_diag::offline::WavReadStatus::malformed_stream:
        default:
            return 4;
        }
    }
}
