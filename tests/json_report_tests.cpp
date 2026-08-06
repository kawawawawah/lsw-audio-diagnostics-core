// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include <limits>
#include <cmath>

namespace lsw::audio_diag::test
{
    LSW_TEST_CASE(JsonReport_Basic)
    {
        lsw::audio_diag::offline::ReportModel model;
        model.audio.sampleRate = 48000;
        model.analysis.activeChannelCount = 1;
        model.analysis.channels[0].samplePeak = 0.5;
        // Setting an inf value to verify it serializes as null
        model.analysis.channels[0].dcOffset = std::numeric_limits<double>::infinity();
        
        std::string json = lsw::audio_diag::offline::generateJsonReport(model, false);
        LSW_CHECK(json.find("\"schemaVersion\":1") != std::string::npos);
        LSW_CHECK(json.find("\"samplePeak\":0.5") != std::string::npos);
        LSW_CHECK(json.find("\"dcOffset\":null") != std::string::npos);
    }
}
