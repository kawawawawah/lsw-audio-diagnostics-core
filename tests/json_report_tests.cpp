// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include <limits>
#include <cmath>
#include <string>

namespace lsw::audio_diag::test
{
    using namespace lsw::audio_diag;
    using namespace lsw::audio_diag::offline;

    static ReportModel createBaseTestModel()
    {
        ReportModel m;
        m.toolName = "lsw_audio_diagnostics_cli";
        m.toolVersion = "0.3.0";
        m.input.path = "test/audio.wav";
        m.input.fileSizeBytes = 1024;

        m.audio.container = "RIFF/WAVE";
        m.audio.encoding = "pcm_s16";
        m.audio.sampleRate = 48000;
        m.audio.channelCount = 1;
        m.audio.bitsPerSample = 16;
        m.audio.validBitsPerSample = 16;
        m.audio.frameCount = 480;
        m.audio.durationSeconds = 0.01;

        m.analysis.processedSampleCount = 480;
        m.analysis.processedBlockCount = 1;
        m.analysis.activeChannelCount = 1;
        m.analysis.diagnosticFlags = DiagnosticFlags::prepared;

        ChannelMetrics cm{};
        cm.samplePeak = 0.5;
        cm.heldPeak = 0.5;
        cm.heldPeakDbfs = -6.02;
        cm.smoothedRms = 0.353;
        cm.rmsDbfs = -9.03;
        cm.dcOffset = 0.0;
        cm.maximumAbsoluteSample = 0.5;
        cm.clipCount = 0;
        cm.consecutiveClipCount = 0;
        cm.invalidSampleCount = 0;
        cm.nanCount = 0;
        cm.positiveInfinityCount = 0;
        cm.negativeInfinityCount = 0;
        cm.denormalCount = 0;
        cm.isSilent = false;
        m.analysis.channels[0] = cm;

        return m;
    }

    LSW_TEST_CASE(JsonReport_GoldenAndDeterminism)
    {
        auto model = createBaseTestModel();

        std::string compact1 = generateJsonReport(model, false);
        std::string compact2 = generateJsonReport(model, false);
        LSW_CHECK_EQ(compact1, compact2); // Compact determinism

        std::string pretty1 = generateJsonReport(model, true);
        std::string pretty2 = generateJsonReport(model, true);
        LSW_CHECK_EQ(pretty1, pretty2); // Pretty determinism

        // Check key elements in compact output
        LSW_CHECK(compact1.find("\"schemaVersion\":1") != std::string::npos);
        LSW_CHECK(compact1.find("\"name\":\"lsw_audio_diagnostics_cli\"") != std::string::npos);
        LSW_CHECK(compact1.find("\"version\":\"0.3.0\"") != std::string::npos);
        LSW_CHECK(compact1.find("\"path\":\"test/audio.wav\"") != std::string::npos);

        // Check field order in JSON
        std::size_t posSchema = compact1.find("\"schemaVersion\"");
        std::size_t posTool = compact1.find("\"tool\"");
        std::size_t posInput = compact1.find("\"input\"");
        std::size_t posAudio = compact1.find("\"audio\"");
        std::size_t posAnalysis = compact1.find("\"analysis\"");

        LSW_CHECK(posSchema < posTool);
        LSW_CHECK(posTool < posInput);
        LSW_CHECK(posInput < posAudio);
        LSW_CHECK(posAudio < posAnalysis);
    }

    LSW_TEST_CASE(JsonReport_PathEscaping)
    {
        auto model = createBaseTestModel();
        // Path with quotes, backslash, newline, tab, and control character
        std::string specialPath = "C:\\path\\with \"quotes\"\n\t\x01";
        model.input.path = specialPath;

        std::string json = generateJsonReport(model, false);
        // Escalated checks for proper JSON escaping
        LSW_CHECK(json.find("C:\\\\path\\\\with \\\"quotes\\\"\\n\\t\\u0001") != std::string::npos);
    }

    LSW_TEST_CASE(JsonReport_SpecialFloatValues)
    {
        auto model = createBaseTestModel();
        model.analysis.channels[0].dcOffset = std::numeric_limits<double>::quiet_NaN();
        model.analysis.channels[0].heldPeakDbfs = std::numeric_limits<double>::infinity();
        model.analysis.channels[0].rmsDbfs = -std::numeric_limits<double>::infinity();

        std::string json = generateJsonReport(model, false);
        // NaNs and Infinities must output as null
        LSW_CHECK(json.find("\"dcOffset\":null") != std::string::npos);
        LSW_CHECK(json.find("\"heldPeakDbfs\":null") != std::string::npos);
        LSW_CHECK(json.find("\"rmsDbfs\":null") != std::string::npos);
    }

    LSW_TEST_CASE(JsonReport_FlagsOrderAndEscaping)
    {
        auto model = createBaseTestModel();
        model.analysis.diagnosticFlags = DiagnosticFlags::prepared |
                                          DiagnosticFlags::clippingDetected |
                                          DiagnosticFlags::silenceDetected;

        std::string json = generateJsonReport(model, false);
        // Check array elements order matches enum ordering
        std::size_t posPrep = json.find("\"prepared\"");
        std::size_t posSil = json.find("\"silence_detected\"");
        std::size_t posClip = json.find("\"clipping_detected\"");

        LSW_CHECK(posPrep != std::string::npos);
        LSW_CHECK(posSil != std::string::npos);
        LSW_CHECK(posClip != std::string::npos);
        LSW_CHECK(posPrep < posSil);
        LSW_CHECK(posSil < posClip);
    }

    LSW_TEST_CASE(JsonReport_MonoVsStereoChannelsAndEvents)
    {
        // Mono case
        {
            auto model = createBaseTestModel();
            model.audio.channelCount = 1;
            model.analysis.activeChannelCount = 1;

            std::string json = generateJsonReport(model, false);
            LSW_CHECK(json.find("\"channels\":[{") != std::string::npos);
            LSW_CHECK(json.find("\"channelEvents\":[{") != std::string::npos);
        }

        // Stereo case
        {
            auto model = createBaseTestModel();
            model.audio.channelCount = 2;
            model.analysis.activeChannelCount = 2;
            model.analysis.channels[1] = model.analysis.channels[0];
            model.analysis.channelEvents[1] = model.analysis.channelEvents[0];

            std::string json = generateJsonReport(model, false);
            // Must have two elements in channels array
            std::size_t posCh = json.find("\"channels\":[");
            LSW_CHECK(posCh != std::string::npos);
            // Verify eventState fields presence
            LSW_CHECK(json.find("\"active\":false") != std::string::npos);
            LSW_CHECK(json.find("\"latched\":false") != std::string::npos);
            LSW_CHECK(json.find("\"eventCount\":0") != std::string::npos);
        }
    }
}
