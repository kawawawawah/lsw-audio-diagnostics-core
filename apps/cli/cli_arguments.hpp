// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

namespace lsw::audio_diag::cli
{
    enum class Command
    {
        help,
        version,
        analyze
    };

    struct ParseResult
    {
        bool success;
        int exitCode;
        std::string errorMessage;

        Command command;
        std::string inputPath;
        std::string outputPath;
        std::size_t blockSize = 4096;
        bool pretty = false;

        static ParseResult makeError(int code, const std::string& msg)
        {
            ParseResult res;
            res.success = false;
            res.exitCode = code;
            res.errorMessage = msg;
            res.command = Command::help;
            return res;
        }

        static ParseResult makeSuccess(Command cmd)
        {
            ParseResult res;
            res.success = true;
            res.exitCode = 0;
            res.command = cmd;
            return res;
        }
    };

    ParseResult parseCliArguments(const std::vector<std::string>& args);
    void printHelp();
}
