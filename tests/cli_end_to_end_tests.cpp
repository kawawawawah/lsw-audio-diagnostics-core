// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include <cstdlib>
#include <string>

#ifndef CLI_EXECUTABLE_PATH
#define CLI_EXECUTABLE_PATH ""
#endif

namespace lsw::audio_diag::test
{
    LSW_TEST_CASE(CliEndToEnd_Help)
    {
#ifdef CLI_EXECUTABLE_PATH
        std::string cmd = std::string("\"") + CLI_EXECUTABLE_PATH + "\" --help >nul 2>nul";
        int ret = std::system(cmd.c_str());
        LSW_CHECK_EQ(ret, 0);
#endif
    }
}
