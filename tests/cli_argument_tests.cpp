// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "../apps/cli/cli_arguments.hpp"
#include <vector>
#include <string>

namespace lsw::audio_diag::test
{
    using namespace lsw::audio_diag::cli;

    LSW_TEST_CASE(CliArguments_EmptyArgs)
    {
        auto res = parseCliArguments({});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_Help)
    {
        auto res = parseCliArguments({"--help"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK(res.command == Command::help);
    }

    LSW_TEST_CASE(CliArguments_HelpExtra)
    {
        auto res = parseCliArguments({"--help", "extra"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_Version)
    {
        auto res = parseCliArguments({"--version"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK(res.command == Command::version);
    }

    LSW_TEST_CASE(CliArguments_VersionExtra)
    {
        auto res = parseCliArguments({"--version", "extra"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_MissingAnalyze)
    {
        auto res = parseCliArguments({"unknown_cmd"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_AnalyzeWithoutInput)
    {
        auto res = parseCliArguments({"analyze"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_InputStartsWithDash)
    {
        auto res = parseCliArguments({"analyze", "-input.wav"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_ValidMinimalAnalyze)
    {
        auto res = parseCliArguments({"analyze", "input.wav"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK(res.command == Command::analyze);
        LSW_CHECK_EQ(res.inputPath, std::string("input.wav"));
        LSW_CHECK(res.outputPath.empty());
        LSW_CHECK(!res.pretty);
        LSW_CHECK_EQ(res.blockSize, 4096ULL);
    }

    LSW_TEST_CASE(CliArguments_UnknownOption)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--unknown"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_DuplicateOutput)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--output", "out1.json", "--output", "out2.json"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_MissingOutputValue)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--output"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_OutputFollowedByOption)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--output", "--pretty"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_DuplicatePretty)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--pretty", "--pretty"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_DuplicateBlockSize)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "1024", "--block-size", "2048"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_MissingBlockSizeValue)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_BlockSizeNonNumber)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "abc"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_BlockSizeTrailingChars)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "4096abc"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_BlockSizeNegative)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "-100"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_BlockSizeZero)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "0"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_BlockSize1)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "1"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK_EQ(res.blockSize, 1ULL);
    }

    LSW_TEST_CASE(CliArguments_BlockSize4096)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "4096"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK_EQ(res.blockSize, 4096ULL);
    }

    LSW_TEST_CASE(CliArguments_BlockSize1048576)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "1048576"});
        LSW_CHECK(res.success);
        LSW_CHECK_EQ(res.exitCode, 0);
        LSW_CHECK_EQ(res.blockSize, 1048576ULL);
    }

    LSW_TEST_CASE(CliArguments_BlockSize1048577)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "--block-size", "1048577"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }

    LSW_TEST_CASE(CliArguments_ExtraPositionalArgument)
    {
        auto res = parseCliArguments({"analyze", "input.wav", "extra.wav"});
        LSW_CHECK(!res.success);
        LSW_CHECK_EQ(res.exitCode, 2);
    }
}
