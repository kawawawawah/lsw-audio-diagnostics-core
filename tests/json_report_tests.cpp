// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "test_framework.hpp"
#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include <limits>
#include <cmath>
#include <string>
#include <cstdio>

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
        LSW_CHECK_EQ(compact1, compact2);

        std::string pretty1 = generateJsonReport(model, true);
        std::string pretty2 = generateJsonReport(model, true);
        LSW_CHECK_EQ(pretty1, pretty2);

        // Exact full golden string verification for compact
        std::string expectedCompact =
            "{\"schemaVersion\":1,\"tool\":{\"name\":\"lsw_audio_diagnostics_cli\",\"version\":\"0.3.0\"},"
            "\"input\":{\"path\":\"test/audio.wav\",\"fileSizeBytes\":1024},"
            "\"audio\":{\"container\":\"RIFF/WAVE\",\"encoding\":\"pcm_s16\",\"sampleRate\":48000,\"channelCount\":1,\"bitsPerSample\":16,\"validBitsPerSample\":16,\"frameCount\":480,\"durationSeconds\":0.01},"
            "\"analysis\":{\"processedFrameCount\":480,\"processedBlockCount\":1,\"diagnosticFlags\":[\"prepared\"],"
            "\"channels\":[{\"samplePeak\":0.5,\"heldPeak\":0.5,\"heldPeakDbfs\":-6.02,\"smoothedRms\":0.353,\"rmsDbfs\":-9.03,\"dcOffset\":0,\"maximumAbsoluteSample\":0.5,\"clipCount\":0,\"consecutiveClipCount\":0,\"invalidSampleCount\":0,\"nanCount\":0,\"positiveInfinityCount\":0,\"negativeInfinityCount\":0,\"denormalCount\":0,\"isSilent\":false}],"
            "\"stereo\":{\"correlation\":0,\"leftRms\":0,\"rightRms\":0,\"channelBalanceDb\":0,\"monoCompatibilityScore\":0.5,\"identicalChannels\":false,\"reversedPolarity\":false,\"leftOnly\":false,\"rightOnly\":false},"
            "\"channelEvents\":[{\"dropout\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"sustainedClip\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"dcFault\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"invalidBurst\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"maximumObservedDcOffset\":0,\"maximumInvalidSamplesPerBlock\":0}],"
            "\"stereoEvents\":{\"reversedPolarity\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"identicalChannels\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"leftOnly\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0},"
            "\"rightOnly\":{\"active\":false,\"latched\":false,\"eventCount\":0,\"currentDurationSamples\":0,\"longestDurationSamples\":0,\"lastStartedAtSample\":0}}}}";

        LSW_CHECK_EQ(compact1, expectedCompact);

        // Exact full golden string verification for pretty
        std::string expectedPretty =
            "{\n"
            "  \"schemaVersion\": 1,\n"
            "  \"tool\": {\n"
            "    \"name\": \"lsw_audio_diagnostics_cli\",\n"
            "    \"version\": \"0.3.0\"\n"
            "  },\n"
            "  \"input\": {\n"
            "    \"path\": \"test/audio.wav\",\n"
            "    \"fileSizeBytes\": 1024\n"
            "  },\n"
            "  \"audio\": {\n"
            "    \"container\": \"RIFF/WAVE\",\n"
            "    \"encoding\": \"pcm_s16\",\n"
            "    \"sampleRate\": 48000,\n"
            "    \"channelCount\": 1,\n"
            "    \"bitsPerSample\": 16,\n"
            "    \"validBitsPerSample\": 16,\n"
            "    \"frameCount\": 480,\n"
            "    \"durationSeconds\": 0.01\n"
            "  },\n"
            "  \"analysis\": {\n"
            "    \"processedFrameCount\": 480,\n"
            "    \"processedBlockCount\": 1,\n"
            "    \"diagnosticFlags\": [\n"
            "      \"prepared\"\n"
            "    ],\n"
            "    \"channels\": [\n"
            "      {\n"
            "        \"samplePeak\": 0.5,\n"
            "        \"heldPeak\": 0.5,\n"
            "        \"heldPeakDbfs\": -6.02,\n"
            "        \"smoothedRms\": 0.353,\n"
            "        \"rmsDbfs\": -9.03,\n"
            "        \"dcOffset\": 0,\n"
            "        \"maximumAbsoluteSample\": 0.5,\n"
            "        \"clipCount\": 0,\n"
            "        \"consecutiveClipCount\": 0,\n"
            "        \"invalidSampleCount\": 0,\n"
            "        \"nanCount\": 0,\n"
            "        \"positiveInfinityCount\": 0,\n"
            "        \"negativeInfinityCount\": 0,\n"
            "        \"denormalCount\": 0,\n"
            "        \"isSilent\": false\n"
            "      }\n"
            "    ],\n"
            "    \"stereo\": {\n"
            "      \"correlation\": 0,\n"
            "      \"leftRms\": 0,\n"
            "      \"rightRms\": 0,\n"
            "      \"channelBalanceDb\": 0,\n"
            "      \"monoCompatibilityScore\": 0.5,\n"
            "      \"identicalChannels\": false,\n"
            "      \"reversedPolarity\": false,\n"
            "      \"leftOnly\": false,\n"
            "      \"rightOnly\": false\n"
            "    },\n"
            "    \"channelEvents\": [\n"
            "      {\n"
            "        \"dropout\": {\n"
            "          \"active\": false,\n"
            "          \"latched\": false,\n"
            "          \"eventCount\": 0,\n"
            "          \"currentDurationSamples\": 0,\n"
            "          \"longestDurationSamples\": 0,\n"
            "          \"lastStartedAtSample\": 0\n"
            "        },\n"
            "        \"sustainedClip\": {\n"
            "          \"active\": false,\n"
            "          \"latched\": false,\n"
            "          \"eventCount\": 0,\n"
            "          \"currentDurationSamples\": 0,\n"
            "          \"longestDurationSamples\": 0,\n"
            "          \"lastStartedAtSample\": 0\n"
            "        },\n"
            "        \"dcFault\": {\n"
            "          \"active\": false,\n"
            "          \"latched\": false,\n"
            "          \"eventCount\": 0,\n"
            "          \"currentDurationSamples\": 0,\n"
            "          \"longestDurationSamples\": 0,\n"
            "          \"lastStartedAtSample\": 0\n"
            "        },\n"
            "        \"invalidBurst\": {\n"
            "          \"active\": false,\n"
            "          \"latched\": false,\n"
            "          \"eventCount\": 0,\n"
            "          \"currentDurationSamples\": 0,\n"
            "          \"longestDurationSamples\": 0,\n"
            "          \"lastStartedAtSample\": 0\n"
            "        },\n"
            "        \"maximumObservedDcOffset\": 0,\n"
            "        \"maximumInvalidSamplesPerBlock\": 0\n"
            "      }\n"
            "    ],\n"
            "    \"stereoEvents\": {\n"
            "      \"reversedPolarity\": {\n"
            "        \"active\": false,\n"
            "        \"latched\": false,\n"
            "        \"eventCount\": 0,\n"
            "        \"currentDurationSamples\": 0,\n"
            "        \"longestDurationSamples\": 0,\n"
            "        \"lastStartedAtSample\": 0\n"
            "      },\n"
            "      \"identicalChannels\": {\n"
            "        \"active\": false,\n"
            "        \"latched\": false,\n"
            "        \"eventCount\": 0,\n"
            "        \"currentDurationSamples\": 0,\n"
            "        \"longestDurationSamples\": 0,\n"
            "        \"lastStartedAtSample\": 0\n"
            "      },\n"
            "      \"leftOnly\": {\n"
            "        \"active\": false,\n"
            "        \"latched\": false,\n"
            "        \"eventCount\": 0,\n"
            "        \"currentDurationSamples\": 0,\n"
            "        \"longestDurationSamples\": 0,\n"
            "        \"lastStartedAtSample\": 0\n"
            "      },\n"
            "      \"rightOnly\": {\n"
            "        \"active\": false,\n"
            "        \"latched\": false,\n"
            "        \"eventCount\": 0,\n"
            "        \"currentDurationSamples\": 0,\n"
            "        \"longestDurationSamples\": 0,\n"
            "        \"lastStartedAtSample\": 0\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "}\n";

        LSW_CHECK_EQ(pretty1, expectedPretty);

        // Check top-level field order in JSON
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

    LSW_TEST_CASE(JsonReport_ControlCharactersLoop)
    {
        // Loop over all control characters U+0000 to U+001F
        for (int c = 0; c <= 0x1F; ++c)
        {
            auto model = createBaseTestModel();
            std::string s;
            s.push_back(static_cast<char>(c));
            model.input.path = s;

            std::string json = generateJsonReport(model, false);
            LSW_CHECK(!json.empty());

            // Build expected escape sequence
            std::string expectedEscape;
            switch (c)
            {
            case 0x08: expectedEscape = "\\b"; break;
            case 0x09: expectedEscape = "\\t"; break;
            case 0x0A: expectedEscape = "\\n"; break;
            case 0x0C: expectedEscape = "\\f"; break;
            case 0x0D: expectedEscape = "\\r"; break;
            default:
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u00%02x", c);
                    expectedEscape = buf;
                }
                break;
            }

            // Verify expected escape sequence is present in JSON
            LSW_CHECK(json.find(expectedEscape) != std::string::npos);

            // Verify raw unescaped control character is NOT present in JSON
            std::string rawControl(1, static_cast<char>(c));
            LSW_CHECK(json.find(rawControl) == std::string::npos);
        }
    }

    LSW_TEST_CASE(JsonReport_SpecialFloatValues)
    {
        auto model = createBaseTestModel();
        model.analysis.channels[0].dcOffset = std::numeric_limits<double>::quiet_NaN();
        model.analysis.channels[0].heldPeakDbfs = std::numeric_limits<double>::infinity();
        model.analysis.channels[0].rmsDbfs = -std::numeric_limits<double>::infinity();

        std::string json = generateJsonReport(model, false);
        LSW_CHECK(json.find("\"dcOffset\":null") != std::string::npos);
        LSW_CHECK(json.find("\"heldPeakDbfs\":null") != std::string::npos);
        LSW_CHECK(json.find("\"rmsDbfs\":null") != std::string::npos);
    }

    LSW_TEST_CASE(JsonReport_FlagsOrderAndEmptyFlags)
    {
        // Empty flags
        {
            auto model = createBaseTestModel();
            model.analysis.diagnosticFlags = DiagnosticFlags::none;
            std::string json = generateJsonReport(model, false);
            LSW_CHECK(json.find("\"diagnosticFlags\":[]") != std::string::npos);
        }

        // Flags in enum order
        {
            auto model = createBaseTestModel();
            model.analysis.diagnosticFlags = DiagnosticFlags::prepared |
                                              DiagnosticFlags::clippingDetected |
                                              DiagnosticFlags::silenceDetected;

            std::string json = generateJsonReport(model, false);
            std::size_t posPrep = json.find("\"prepared\"");
            std::size_t posSil = json.find("\"silence_detected\"");
            std::size_t posClip = json.find("\"clipping_detected\"");

            LSW_CHECK(posPrep != std::string::npos);
            LSW_CHECK(posSil != std::string::npos);
            LSW_CHECK(posClip != std::string::npos);
            LSW_CHECK(posPrep < posSil);
            LSW_CHECK(posSil < posClip);
        }
    }

    LSW_TEST_CASE(JsonReport_MonoVsStereoChannelsAndEvents)
    {
        // Mono case (neutral stereo metrics)
        {
            auto model = createBaseTestModel();
            model.audio.channelCount = 1;
            model.analysis.activeChannelCount = 1;

            std::string json = generateJsonReport(model, false);
            LSW_CHECK(json.find("\"channels\":[{") != std::string::npos);
            LSW_CHECK(json.find("\"channelEvents\":[{") != std::string::npos);
        }

        // Stereo case (2 channels)
        {
            auto model = createBaseTestModel();
            model.audio.channelCount = 2;
            model.analysis.activeChannelCount = 2;
            model.analysis.channels[1] = model.analysis.channels[0];
            model.analysis.channelEvents[1] = model.analysis.channelEvents[0];

            std::string json = generateJsonReport(model, false);
            LSW_CHECK(json.find("\"active\":false") != std::string::npos);
            LSW_CHECK(json.find("\"latched\":false") != std::string::npos);
            LSW_CHECK(json.find("\"eventCount\":0") != std::string::npos);
        }
    }
}
