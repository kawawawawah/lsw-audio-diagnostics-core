// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"
#include "lsw/audio_diag/diagnostic_flags.hpp"
#include "synthetic_signal.hpp"
#include "test_framework.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{
    [[nodiscard]] lsw::audio_diag::AnalyzerConfig eventConfig(const std::size_t channels = 1U,
                                                               const std::size_t maximumBlockSize = 32U)
    {
        auto config = lsw::audio_diag::test::quickConfig(channels, maximumBlockSize);
        config.sampleRate = 1000.0;
        config.silenceThresholdDbfs = -80.0;
        config.dropoutThresholdDbfs = -60.0;
        config.activeSignalThresholdDbfs = -20.0;
        config.peakHoldSeconds = 0.002;
        config.peakHoldDecayDbPerSecond = 20.0;
        config.dropoutHoldSeconds = 0.010;
        config.dropoutRecoverySeconds = 0.005;
        config.sustainedClipMinimumSamples = 3U;
        config.dcFaultThreshold = 0.10;
        config.dcFaultHoldSeconds = 0.010;
        config.dcFaultRecoverySeconds = 0.005;
        config.invalidBurstThresholdPerBlock = 2U;
        return config;
    }

    template <typename SampleType>
    [[nodiscard]] lsw::audio_diag::Analyzer<SampleType> prepared(const lsw::audio_diag::AnalyzerConfig& config)
    {
        lsw::audio_diag::Analyzer<SampleType> analyzer;
        if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
        {
            return {};
        }
        return analyzer;
    }

    template <typename SampleType>
    void processMono(lsw::audio_diag::Analyzer<SampleType>& analyzer,
                     const std::vector<SampleType>& input) noexcept
    {
        const SampleType* channels[] { input.data() };
        analyzer.process(channels, 1U, input.size());
    }

    void processStereo(lsw::audio_diag::Analyzer<float>& analyzer, const std::vector<float>& left,
                       const std::vector<float>& right) noexcept
    {
        const float* channels[] { left.data(), right.data() };
        analyzer.process(channels, 2U, left.size());
    }

    [[nodiscard]] std::vector<float> activeBlock()
    {
        return lsw::audio_diag::test::constant<float>(10U, 0.5);
    }

    [[nodiscard]] std::vector<float> silentBlock()
    {
        return lsw::audio_diag::test::silence<float>(10U);
    }
}

LSW_TEST_CASE(event_state_defaults_are_neutral)
{
    const lsw::audio_diag::EventState state {};
    LSW_CHECK(!state.active);
    LSW_CHECK(!state.latched);
    LSW_CHECK_EQ(state.eventCount, 0U);
}

LSW_TEST_CASE(peak_hold_updates_immediately)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 0.75F });
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_NEAR(snapshot.channels[0].heldPeak, 0.75, 1.0e-6);
    LSW_CHECK_NEAR(snapshot.channels[0].heldPeakDbfs, -2.4988, 0.01);
}

LSW_TEST_CASE(peak_hold_survives_its_hold_interval)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F });
    processMono(analyzer, { 0.0F });
    LSW_CHECK_NEAR(analyzer.getSnapshot().channels[0].heldPeak, 1.0, 1.0e-6);
}

LSW_TEST_CASE(peak_hold_decays_after_the_hold_interval)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F });
    processMono(analyzer, { 0.0F });
    processMono(analyzer, { 0.0F });
    processMono(analyzer, { 0.0F });
    LSW_CHECK(analyzer.getSnapshot().channels[0].heldPeak < 1.0);
}

LSW_TEST_CASE(zero_peak_hold_starts_decay_without_delay)
{
    auto config = eventConfig();
    config.peakHoldSeconds = 0.0;
    auto analyzer = prepared<float>(config);
    processMono(analyzer, { 1.0F });
    processMono(analyzer, { 0.0F });
    LSW_CHECK(analyzer.getSnapshot().channels[0].heldPeak < 1.0);
}

LSW_TEST_CASE(peak_hold_values_stay_finite)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.5F });
    const auto metrics = analyzer.getSnapshot().channels[0];
    LSW_CHECK(std::isfinite(metrics.heldPeak));
    LSW_CHECK(std::isfinite(metrics.heldPeakDbfs));
    LSW_CHECK(metrics.heldPeakDbfs > 0.0);
}

LSW_TEST_CASE(peak_hold_resets_with_reset_levels)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F });
    analyzer.resetLevels();
    LSW_CHECK_NEAR(analyzer.getSnapshot().channels[0].heldPeak, 0.0, 1.0e-12);
}

LSW_TEST_CASE(peak_hold_works_for_double)
{
    auto analyzer = prepared<double>(eventConfig());
    processMono(analyzer, { 0.5 });
    LSW_CHECK_NEAR(analyzer.getSnapshot().channels[0].heldPeak, 0.5, 1.0e-12);
}

LSW_TEST_CASE(initial_silence_does_not_create_a_dropout)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, silentBlock());
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].dropout.eventCount, 0U);
}

LSW_TEST_CASE(active_signal_arms_the_dropout_detector)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    LSW_CHECK(analyzer.getSnapshot().channelEvents[0].dropout.active);
}

LSW_TEST_CASE(dropout_starts_at_the_exact_hold_boundary)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    const auto event = analyzer.getSnapshot().channelEvents[0].dropout;
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 10U);
    LSW_CHECK_EQ(event.lastStartedAtSample, 10U);
}

LSW_TEST_CASE(continuous_dropout_is_one_event)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    processMono(analyzer, silentBlock());
    const auto event = analyzer.getSnapshot().channelEvents[0].dropout;
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 20U);
}

LSW_TEST_CASE(dropout_recovers_after_active_signal)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    processMono(analyzer, activeBlock());
    LSW_CHECK(!analyzer.getSnapshot().channelEvents[0].dropout.active);
}

LSW_TEST_CASE(second_dropout_increments_the_event_count)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].dropout.eventCount, 2U);
}

LSW_TEST_CASE(dropout_latches_and_tracks_longest_duration)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    processMono(analyzer, silentBlock());
    const auto event = analyzer.getSnapshot().channelEvents[0].dropout;
    LSW_CHECK(event.latched);
    LSW_CHECK_EQ(event.longestDurationSamples, 20U);
}

LSW_TEST_CASE(clear_events_does_not_recount_an_active_dropout)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    analyzer.clearEvents();
    processMono(analyzer, silentBlock());
    const auto event = analyzer.getSnapshot().channelEvents[0].dropout;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 0U);
    LSW_CHECK(!event.latched);
}

LSW_TEST_CASE(invalid_dropout_inputs_do_not_create_events)
{
    auto analyzer = prepared<float>(eventConfig());
    analyzer.process(nullptr, 1U, 10U);
    const float* channels[] { nullptr };
    analyzer.process(channels, 1U, 10U);
    analyzer.process(channels, 1U, 0U);
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].dropout.eventCount, 0U);
}

LSW_TEST_CASE(short_clip_run_does_not_start_an_event)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F });
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].sustainedClip.eventCount, 0U);
}

LSW_TEST_CASE(exact_clip_run_starts_an_event)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F });
    const auto event = analyzer.getSnapshot().channelEvents[0].sustainedClip;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 3U);
}

LSW_TEST_CASE(clip_run_crosses_blocks_and_remembers_its_start)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F });
    processMono(analyzer, { 1.0F, 1.0F });
    const auto event = analyzer.getSnapshot().channelEvents[0].sustainedClip;
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.lastStartedAtSample, 0U);
    LSW_CHECK_EQ(event.longestDurationSamples, 4U);
}

LSW_TEST_CASE(nonclip_sample_ends_a_sustained_clip_event)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F, 0.0F });
    LSW_CHECK(!analyzer.getSnapshot().channelEvents[0].sustainedClip.active);
}

LSW_TEST_CASE(second_clip_run_increments_the_event_count)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F, 0.0F, 1.0F, 1.0F, 1.0F });
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].sustainedClip.eventCount, 2U);
}

LSW_TEST_CASE(clip_threshold_one_sample_is_supported)
{
    auto config = eventConfig();
    config.sustainedClipMinimumSamples = 1U;
    auto analyzer = prepared<float>(config);
    processMono(analyzer, { 1.0F });
    LSW_CHECK(analyzer.getSnapshot().channelEvents[0].sustainedClip.active);
}

LSW_TEST_CASE(clear_events_preserves_an_active_sustained_clip)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F });
    analyzer.clearEvents();
    processMono(analyzer, { 1.0F });
    const auto event = analyzer.getSnapshot().channelEvents[0].sustainedClip;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 0U);
}

LSW_TEST_CASE(reset_counters_does_not_clear_sustained_clip_state)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F });
    analyzer.resetCounters();
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(snapshot.channelEvents[0].sustainedClip.active);
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 0U);
}

LSW_TEST_CASE(dc_fault_requires_active_signal)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, silentBlock());
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].dcFault.eventCount, 0U);
}

LSW_TEST_CASE(dc_fault_starts_at_the_hold_boundary)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::constant<float>(10U, 0.35));
    const auto event = analyzer.getSnapshot().channelEvents[0].dcFault;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 10U);
}

LSW_TEST_CASE(negative_dc_fault_is_detected)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::constant<float>(10U, -0.35));
    LSW_CHECK(analyzer.getSnapshot().channelEvents[0].dcFault.active);
}

LSW_TEST_CASE(dc_fault_recovers_when_dc_is_removed)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::constant<float>(10U, 0.35));
    processMono(analyzer, silentBlock());
    LSW_CHECK(!analyzer.getSnapshot().channelEvents[0].dcFault.active);
}

LSW_TEST_CASE(dc_fault_tracks_the_maximum_absolute_offset)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::constant<float>(10U, -0.35));
    LSW_CHECK(analyzer.getSnapshot().channelEvents[0].maximumObservedDcOffset >= 0.34);
}

LSW_TEST_CASE(reset_levels_rebaselines_dc_fault_without_clearing_history)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::constant<float>(10U, 0.35));
    analyzer.resetLevels();
    const auto event = analyzer.getSnapshot().channelEvents[0].dcFault;
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK(!event.active);
}

LSW_TEST_CASE(invalid_burst_requires_the_configured_number_of_samples)
{
    auto analyzer = prepared<float>(eventConfig());
    auto input = lsw::audio_diag::test::constant<float>(4U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    processMono(analyzer, input);
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].invalidBurst.eventCount, 0U);
}

LSW_TEST_CASE(invalid_burst_counts_nan_and_infinities)
{
    auto analyzer = prepared<float>(eventConfig());
    auto input = lsw::audio_diag::test::constant<float>(4U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    input[1] = std::numeric_limits<float>::infinity();
    input[2] = -std::numeric_limits<float>::infinity();
    processMono(analyzer, input);
    const auto event = analyzer.getSnapshot().channelEvents[0].invalidBurst;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 4U);
}

LSW_TEST_CASE(denormals_do_not_trigger_invalid_bursts)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, lsw::audio_diag::test::denormalSignal<float>(4U));
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.channelEvents[0].invalidBurst.eventCount, 0U);
    LSW_CHECK(std::isfinite(snapshot.channels[0].rmsDbfs));
}

LSW_TEST_CASE(clean_block_ends_an_invalid_burst)
{
    auto analyzer = prepared<float>(eventConfig());
    auto input = lsw::audio_diag::test::constant<float>(4U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    input[1] = std::numeric_limits<float>::infinity();
    processMono(analyzer, input);
    processMono(analyzer, lsw::audio_diag::test::constant<float>(4U, 0.25));
    LSW_CHECK(!analyzer.getSnapshot().channelEvents[0].invalidBurst.active);
}

LSW_TEST_CASE(invalid_burst_tracks_its_largest_block)
{
    auto analyzer = prepared<float>(eventConfig());
    auto input = lsw::audio_diag::test::constant<float>(4U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    input[1] = std::numeric_limits<float>::infinity();
    input[2] = -std::numeric_limits<float>::infinity();
    processMono(analyzer, input);
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].maximumInvalidSamplesPerBlock, 3U);
}

LSW_TEST_CASE(reversed_polarity_creates_one_stereo_event)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = lsw::audio_diag::test::constant<float>(10U, 0.5);
    const auto right = lsw::audio_diag::test::constant<float>(10U, -0.5);
    processStereo(analyzer, left, right);
    const auto event = analyzer.getSnapshot().stereoEvents.reversedPolarity;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 1U);
}

LSW_TEST_CASE(continuous_reversed_polarity_does_not_recount)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = lsw::audio_diag::test::constant<float>(10U, 0.5);
    const auto right = lsw::audio_diag::test::constant<float>(10U, -0.5);
    processStereo(analyzer, left, right);
    processStereo(analyzer, left, right);
    const auto event = analyzer.getSnapshot().stereoEvents.reversedPolarity;
    LSW_CHECK_EQ(event.eventCount, 1U);
    LSW_CHECK_EQ(event.currentDurationSamples, 20U);
}

LSW_TEST_CASE(identical_channels_and_left_only_events_are_detected)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = lsw::audio_diag::test::constant<float>(10U, 0.5);
    processStereo(analyzer, left, left);
    LSW_CHECK(analyzer.getSnapshot().stereoEvents.identicalChannels.active);
    const auto silent = lsw::audio_diag::test::silence<float>(10U);
    processStereo(analyzer, left, silent);
    LSW_CHECK(analyzer.getSnapshot().stereoEvents.leftOnly.active);
}

LSW_TEST_CASE(right_only_stereo_event_is_detected)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto active = lsw::audio_diag::test::constant<float>(10U, 0.5);
    const auto silent = lsw::audio_diag::test::silence<float>(10U);
    processStereo(analyzer, silent, active);
    LSW_CHECK(analyzer.getSnapshot().stereoEvents.rightOnly.active);
}

LSW_TEST_CASE(mono_snapshots_keep_stereo_events_neutral)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, activeBlock());
    const auto events = analyzer.getSnapshot().stereoEvents;
    LSW_CHECK_EQ(events.reversedPolarity.eventCount, 0U);
    LSW_CHECK_EQ(events.identicalChannels.eventCount, 0U);
}

LSW_TEST_CASE(clear_events_preserves_an_active_stereo_state)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = lsw::audio_diag::test::constant<float>(10U, 0.5);
    const auto right = lsw::audio_diag::test::constant<float>(10U, -0.5);
    processStereo(analyzer, left, right);
    analyzer.clearEvents();
    processStereo(analyzer, left, right);
    const auto event = analyzer.getSnapshot().stereoEvents.reversedPolarity;
    LSW_CHECK(event.active);
    LSW_CHECK_EQ(event.eventCount, 0U);
}

LSW_TEST_CASE(stereo_null_input_does_not_generate_an_event)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    analyzer.process(nullptr, 2U, 10U);
    LSW_CHECK_EQ(analyzer.getSnapshot().stereoEvents.reversedPolarity.eventCount, 0U);
}

LSW_TEST_CASE(partial_null_stereo_preserves_available_channel_metrics)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = activeBlock();
    const float* channels[] { left.data(), nullptr };
    analyzer.process(channels, 2U, left.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_NEAR(snapshot.channels[0].samplePeak, 0.5, 1.0e-6);
    LSW_CHECK(snapshot.channels[0].smoothedRms > 0.0);
    LSW_CHECK_EQ(snapshot.processedSampleCount, 10U);
    LSW_CHECK_EQ(snapshot.processedBlockCount, 1U);
    LSW_CHECK(lsw::audio_diag::hasFlag(snapshot.diagnosticFlags,
                                       lsw::audio_diag::DiagnosticFlags::nullInput));
}

LSW_TEST_CASE(partial_null_stereo_suppresses_channel_and_stereo_event_transitions)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto clipped = lsw::audio_diag::test::constant<float>(10U, 1.1);
    const float* channels[] { clipped.data(), nullptr };
    analyzer.process(channels, 2U, clipped.size());
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.channelEvents[0].dropout.eventCount, 0U);
    LSW_CHECK_EQ(snapshot.channelEvents[0].sustainedClip.eventCount, 0U);
    LSW_CHECK_EQ(snapshot.stereoEvents.leftOnly.eventCount, 0U);
    LSW_CHECK_EQ(snapshot.stereoEvents.rightOnly.eventCount, 0U);
}

LSW_TEST_CASE(partial_null_stereo_freezes_active_event_duration_and_state)
{
    auto analyzer = prepared<float>(eventConfig(2U));
    const auto left = lsw::audio_diag::test::constant<float>(10U, 1.1);
    const auto right = lsw::audio_diag::test::constant<float>(10U, -1.1);
    processStereo(analyzer, left, right);
    const auto before = analyzer.getSnapshot();
    const float* partialChannels[] { left.data(), nullptr };
    analyzer.process(partialChannels, 2U, left.size());
    const auto after = analyzer.getSnapshot();
    LSW_CHECK(after.channelEvents[0].sustainedClip.active);
    LSW_CHECK_EQ(after.channelEvents[0].sustainedClip.eventCount,
                 before.channelEvents[0].sustainedClip.eventCount);
    LSW_CHECK_EQ(after.channelEvents[0].sustainedClip.currentDurationSamples,
                 before.channelEvents[0].sustainedClip.currentDurationSamples);
    LSW_CHECK(after.stereoEvents.reversedPolarity.active);
    LSW_CHECK_EQ(after.stereoEvents.reversedPolarity.eventCount,
                 before.stereoEvents.reversedPolarity.eventCount);
    LSW_CHECK_EQ(after.stereoEvents.reversedPolarity.currentDurationSamples,
                 before.stereoEvents.reversedPolarity.currentDurationSamples);
}

LSW_TEST_CASE(dropout_arm_requires_contiguous_active_signal)
{
    auto config = eventConfig();
    config.dropoutRecoverySeconds = 0.020;
    auto analyzer = prepared<float>(config);
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        processMono(analyzer, activeBlock());
        processMono(analyzer, silentBlock());
    }
    LSW_CHECK_EQ(analyzer.getSnapshot().channelEvents[0].dropout.eventCount, 0U);
}

LSW_TEST_CASE(dropout_arm_starts_only_after_contiguous_active_recovery_interval)
{
    auto config = eventConfig();
    config.dropoutRecoverySeconds = 0.020;
    auto analyzer = prepared<float>(config);
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    processMono(analyzer, activeBlock());
    processMono(analyzer, activeBlock());
    processMono(analyzer, silentBlock());
    const auto dropout = analyzer.getSnapshot().channelEvents[0].dropout;
    LSW_CHECK(dropout.active);
    LSW_CHECK_EQ(dropout.eventCount, 1U);
}

LSW_TEST_CASE(reset_clears_events_counters_and_processed_time)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F });
    analyzer.reset();
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK_EQ(snapshot.channels[0].clipCount, 0U);
    LSW_CHECK_EQ(snapshot.channelEvents[0].sustainedClip.eventCount, 0U);
    LSW_CHECK_EQ(snapshot.processedSampleCount, 0U);
}

LSW_TEST_CASE(reset_levels_preserves_counters_and_event_history)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 1.0F, 1.0F, 1.0F });
    const auto before = analyzer.getSnapshot();
    analyzer.resetLevels();
    const auto after = analyzer.getSnapshot();
    LSW_CHECK_EQ(after.channels[0].clipCount, before.channels[0].clipCount);
    LSW_CHECK_EQ(after.channelEvents[0].sustainedClip.eventCount,
                 before.channelEvents[0].sustainedClip.eventCount);
    LSW_CHECK_NEAR(after.channels[0].samplePeak, 0.0, 1.0e-12);
}

LSW_TEST_CASE(clear_diagnostic_flags_only_clears_sticky_bits)
{
    auto analyzer = prepared<float>(eventConfig());
    analyzer.process(nullptr, 1U, 10U);
    analyzer.clearDiagnosticFlags();
    const auto flags = analyzer.getSnapshot().diagnosticFlags;
    LSW_CHECK(!lsw::audio_diag::hasFlag(flags, lsw::audio_diag::DiagnosticFlags::nullInput));
    LSW_CHECK(lsw::audio_diag::hasFlag(flags, lsw::audio_diag::DiagnosticFlags::prepared));
}

LSW_TEST_CASE(clear_events_resets_history_metrics)
{
    auto analyzer = prepared<float>(eventConfig());
    auto input = lsw::audio_diag::test::constant<float>(4U, 0.25);
    input[0] = std::numeric_limits<float>::quiet_NaN();
    input[1] = std::numeric_limits<float>::infinity();
    processMono(analyzer, input);
    analyzer.clearEvents();
    const auto events = analyzer.getSnapshot().channelEvents[0];
    LSW_CHECK_EQ(events.invalidBurst.eventCount, 0U);
    LSW_CHECK_EQ(events.maximumInvalidSamplesPerBlock, 0U);
}

LSW_TEST_CASE(prepare_rejects_invalid_new_event_configuration)
{
    auto config = eventConfig();
    config.peakHoldDecayDbPerSecond = 0.0;
    lsw::audio_diag::Analyzer<float> analyzer;
    LSW_CHECK_EQ(analyzer.prepare(config), lsw::audio_diag::PrepareResult::invalidThreshold);
    config = eventConfig();
    config.sustainedClipMinimumSamples = 0U;
    LSW_CHECK_EQ(analyzer.prepare(config), lsw::audio_diag::PrepareResult::invalidThreshold);
    config = eventConfig();
    config.dropoutThresholdDbfs = -10.0;
    LSW_CHECK_EQ(analyzer.prepare(config), lsw::audio_diag::PrepareResult::invalidThreshold);
}

LSW_TEST_CASE(failed_prepare_keeps_the_previous_valid_state)
{
    auto analyzer = prepared<float>(eventConfig());
    processMono(analyzer, { 0.5F });
    auto invalid = eventConfig();
    invalid.dcFaultThreshold = 0.0;
    LSW_CHECK_EQ(analyzer.prepare(invalid), lsw::audio_diag::PrepareResult::invalidThreshold);
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(snapshot.prepared);
    LSW_CHECK_EQ(snapshot.processedSampleCount, 1U);
}

LSW_TEST_CASE(extreme_finite_samples_keep_all_snapshot_numbers_finite)
{
    auto analyzer = prepared<double>(eventConfig(2U, 1U));
    const double left[] { std::numeric_limits<double>::max() };
    const double right[] { -std::numeric_limits<double>::max() };
    const double* channels[] { left, right };
    analyzer.process(channels, 2U, 1U);
    const auto snapshot = analyzer.getSnapshot();
    LSW_CHECK(std::isfinite(snapshot.channels[0].smoothedRms));
    LSW_CHECK(std::isfinite(snapshot.channels[0].rmsDbfs));
    LSW_CHECK(std::isfinite(snapshot.channels[0].heldPeakDbfs));
    LSW_CHECK(std::isfinite(snapshot.stereo.correlation));
}
