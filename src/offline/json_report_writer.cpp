// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/offline/json_report_writer.hpp"
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>

namespace lsw::audio_diag::offline
{
    namespace
    {
        class JsonWriter
        {
        public:
            JsonWriter(bool pretty) : pretty_(pretty)
            {
                os_.imbue(std::locale::classic());
                os_ << std::setprecision(15);
            }

            std::string str() const { return os_.str(); }

            void beginObject()
            {
                comma();
                os_ << '{';
                ++depth_;
                newline();
                first_ = true;
            }

            void endObject()
            {
                --depth_;
                newline();
                os_ << '}';
                first_ = false;
            }

            void beginArray()
            {
                comma();
                os_ << '[';
                ++depth_;
                newline();
                first_ = true;
            }

            void endArray()
            {
                --depth_;
                newline();
                os_ << ']';
                first_ = false;
            }

            void key(const std::string& k)
            {
                comma();
                writeString(k);
                os_ << (pretty_ ? ": " : ":");
                first_ = true; // Next value shouldn't have a comma
            }

            void value(const std::string& v)
            {
                comma();
                writeString(v);
            }

            void value(const char* v)
            {
                comma();
                writeString(std::string(v));
            }

            void value(double v)
            {
                comma();
                if (std::isfinite(v))
                {
                    os_ << v;
                    if (std::floor(v) == v) {
                        // Ensure it looks like a double, actually we don't strictly need to append .0 
                        // but it might be safer, wait, json doesn't require .0
                    }
                }
                else
                {
                    os_ << "null";
                }
            }

            void value(std::uint64_t v)
            {
                comma();
                os_ << v;
            }
            
            void value(std::uint32_t v)
            {
                comma();
                os_ << v;
            }

            void value(int v)
            {
                comma();
                os_ << v;
            }

            void value(bool v)
            {
                comma();
                os_ << (v ? "true" : "false");
            }

            void valueNull()
            {
                comma();
                os_ << "null";
            }

        private:
            void comma()
            {
                if (!first_)
                {
                    os_ << ',';
                    newline();
                }
                first_ = false;
            }

            void newline()
            {
                if (pretty_)
                {
                    os_ << '\n';
                    for (int i = 0; i < depth_; ++i)
                        os_ << "  ";
                }
            }

            void writeString(const std::string& s)
            {
                os_ << '"';
                for (char c : s)
                {
                    switch (c)
                    {
                    case '"': os_ << "\\\""; break;
                    case '\\': os_ << "\\\\"; break;
                    case '\b': os_ << "\\b"; break;
                    case '\f': os_ << "\\f"; break;
                    case '\n': os_ << "\\n"; break;
                    case '\r': os_ << "\\r"; break;
                    case '\t': os_ << "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) <= 0x1F)
                        {
                            os_ << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec << std::setfill(' ');
                        }
                        else
                        {
                            os_ << c;
                        }
                        break;
                    }
                }
                os_ << '"';
            }

            std::ostringstream os_;
            bool pretty_;
            int depth_ = 0;
            bool first_ = true;
        };

        void writeEventState(JsonWriter& w, const EventState& e)
        {
            w.beginObject();
            w.key("active"); w.value(e.active);
            w.key("latched"); w.value(e.latched);
            w.key("eventCount"); w.value(e.eventCount);
            w.key("currentDurationSamples"); w.value(e.currentDurationSamples);
            w.key("longestDurationSamples"); w.value(e.longestDurationSamples);
            w.key("lastStartedAtSample"); w.value(e.lastStartedAtSample);
            w.endObject();
        }
    }

    std::string generateJsonReport(const ReportModel& model, bool pretty)
    {
        JsonWriter w(pretty);
        w.beginObject();

        w.key("schemaVersion"); w.value(1);

        w.key("tool");
        w.beginObject();
        w.key("name"); w.value(model.toolName);
        w.key("version"); w.value(model.toolVersion);
        w.endObject();

        w.key("input");
        w.beginObject();
        w.key("path"); w.value(model.input.path);
        w.key("fileSizeBytes"); w.value(model.input.fileSizeBytes);
        w.endObject();

        w.key("audio");
        w.beginObject();
        w.key("container"); w.value(model.audio.container);
        w.key("encoding"); w.value(model.audio.encoding);
        w.key("sampleRate"); w.value(model.audio.sampleRate);
        w.key("channelCount"); w.value(model.audio.channelCount);
        w.key("bitsPerSample"); w.value(model.audio.bitsPerSample);
        w.key("validBitsPerSample"); w.value(model.audio.validBitsPerSample);
        w.key("frameCount"); w.value(model.audio.frameCount);
        w.key("durationSeconds"); w.value(model.audio.durationSeconds);
        w.endObject();

        w.key("analysis");
        w.beginObject();
        w.key("processedFrameCount"); w.value(model.analysis.processedSampleCount);
        w.key("processedBlockCount"); w.value(model.analysis.processedBlockCount);

        w.key("diagnosticFlags");
        w.beginArray();
        auto flags = model.analysis.diagnosticFlags;
        if (hasFlag(flags, DiagnosticFlags::prepared)) w.value("prepared");
        if (hasFlag(flags, DiagnosticFlags::silenceDetected)) w.value("silence_detected");
        if (hasFlag(flags, DiagnosticFlags::clippingDetected)) w.value("clipping_detected");
        if (hasFlag(flags, DiagnosticFlags::invalidSampleDetected)) w.value("invalid_sample_detected");
        if (hasFlag(flags, DiagnosticFlags::identicalChannelsDetected)) w.value("identical_channels_detected");
        if (hasFlag(flags, DiagnosticFlags::reversedPolarityDetected)) w.value("reversed_polarity_detected");
        if (hasFlag(flags, DiagnosticFlags::leftOnlyDetected)) w.value("left_only_detected");
        if (hasFlag(flags, DiagnosticFlags::rightOnlyDetected)) w.value("right_only_detected");
        if (hasFlag(flags, DiagnosticFlags::nullInput)) w.value("null_input");
        if (hasFlag(flags, DiagnosticFlags::channelCountMismatch)) w.value("channel_count_mismatch");
        if (hasFlag(flags, DiagnosticFlags::blockSizeExceeded)) w.value("block_size_exceeded");
        w.endArray();

        w.key("channels");
        w.beginArray();
        for (std::size_t i = 0; i < model.analysis.activeChannelCount && i < 2; ++i)
        {
            w.beginObject();
            const auto& c = model.analysis.channels[i];
            w.key("samplePeak"); w.value(c.samplePeak);
            w.key("heldPeak"); w.value(c.heldPeak);
            w.key("heldPeakDbfs"); w.value(c.heldPeakDbfs);
            w.key("smoothedRms"); w.value(c.smoothedRms);
            w.key("rmsDbfs"); w.value(c.rmsDbfs);
            w.key("dcOffset"); w.value(c.dcOffset);
            w.key("maximumAbsoluteSample"); w.value(c.maximumAbsoluteSample);
            w.key("clipCount"); w.value(c.clipCount);
            w.key("consecutiveClipCount"); w.value(c.consecutiveClipCount);
            w.key("invalidSampleCount"); w.value(c.invalidSampleCount);
            w.key("nanCount"); w.value(c.nanCount);
            w.key("positiveInfinityCount"); w.value(c.positiveInfinityCount);
            w.key("negativeInfinityCount"); w.value(c.negativeInfinityCount);
            w.key("denormalCount"); w.value(c.denormalCount);
            w.key("isSilent"); w.value(c.isSilent);
            w.endObject();
        }
        w.endArray();

        w.key("stereo");
        w.beginObject();
        const auto& s = model.analysis.stereo;
        w.key("correlation"); w.value(s.correlation);
        w.key("leftRms"); w.value(s.leftRms);
        w.key("rightRms"); w.value(s.rightRms);
        w.key("channelBalanceDb"); w.value(s.channelBalanceDb);
        w.key("monoCompatibilityScore"); w.value(s.monoCompatibilityScore);
        w.key("identicalChannels"); w.value(s.identicalChannels);
        w.key("reversedPolarity"); w.value(s.reversedPolarity);
        w.key("leftOnly"); w.value(s.leftOnly);
        w.key("rightOnly"); w.value(s.rightOnly);
        w.endObject();

        w.key("channelEvents");
        w.beginArray();
        for (std::size_t i = 0; i < model.analysis.activeChannelCount && i < 2; ++i)
        {
            w.beginObject();
            const auto& e = model.analysis.channelEvents[i];
            w.key("dropout"); writeEventState(w, e.dropout);
            w.key("sustainedClip"); writeEventState(w, e.sustainedClip);
            w.key("dcFault"); writeEventState(w, e.dcFault);
            w.key("invalidBurst"); writeEventState(w, e.invalidBurst);
            w.key("maximumObservedDcOffset"); w.value(e.maximumObservedDcOffset);
            w.key("maximumInvalidSamplesPerBlock"); w.value(e.maximumInvalidSamplesPerBlock);
            w.endObject();
        }
        w.endArray();

        w.key("stereoEvents");
        w.beginObject();
        const auto& se = model.analysis.stereoEvents;
        w.key("reversedPolarity"); writeEventState(w, se.reversedPolarity);
        w.key("identicalChannels"); writeEventState(w, se.identicalChannels);
        w.key("leftOnly"); writeEventState(w, se.leftOnly);
        w.key("rightOnly"); writeEventState(w, se.rightOnly);
        w.endObject();

        w.endObject(); // analysis
        w.endObject(); // root
        
        std::string result = w.str();
        if (pretty) {
            result += "\n";
        }
        return result;
    }
}
