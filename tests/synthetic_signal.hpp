// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/analyzer_config.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace lsw::audio_diag::test
{
    constexpr double pi = 3.14159265358979323846;

    [[nodiscard]] inline lsw::audio_diag::AnalyzerConfig quickConfig(const std::size_t channels = 2U,
                                                                      const std::size_t maximumBlockSize = 8192U)
    {
        lsw::audio_diag::AnalyzerConfig config {};
        config.numberOfChannels = channels;
        config.maximumBlockSize = maximumBlockSize;
        config.levelTimeConstantSeconds = 1.0e-6;
        config.dcTimeConstantSeconds = 1.0e-6;
        config.correlationTimeConstantSeconds = 1.0e-6;
        config.silenceHoldSeconds = 0.0;
        return config;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> silence(const std::size_t count)
    {
        return std::vector<SampleType>(count, static_cast<SampleType>(0.0));
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> constant(const std::size_t count, const double value)
    {
        return std::vector<SampleType>(count, static_cast<SampleType>(value));
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> sine(const std::size_t count,
                                                const double amplitude = 1.0,
                                                const double frequency = 1000.0,
                                                const double sampleRate = 48000.0)
    {
        std::vector<SampleType> result(count);
        for (std::size_t index = 0U; index < count; ++index)
        {
            const double phase = (2.0 * pi * frequency * static_cast<double>(index)) / sampleRate;
            result[index] = static_cast<SampleType>(amplitude * std::sin(phase));
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> clippedSine(const std::size_t count,
                                                       const double amplitude,
                                                       const double limit)
    {
        std::vector<SampleType> result = sine<SampleType>(count, amplitude);
        for (SampleType& sample : result)
        {
            const double value = static_cast<double>(sample);
            sample = static_cast<SampleType>(value > limit ? limit : (value < -limit ? -limit : value));
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> deterministicNoise(const std::size_t count)
    {
        std::vector<SampleType> result(count);
        std::uint32_t state = 0x5a17c9e3U;
        constexpr double divisor = 4294967295.0;
        for (SampleType& sample : result)
        {
            state = (1664525U * state) + 1013904223U;
            const double normalized = static_cast<double>(state) / divisor;
            sample = static_cast<SampleType>((2.0 * normalized) - 1.0);
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> nanSignal(const std::size_t count)
    {
        std::vector<SampleType> result = constant<SampleType>(count, 0.25);
        if (!result.empty())
        {
            result.front() = std::numeric_limits<SampleType>::quiet_NaN();
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> positiveInfinitySignal(const std::size_t count)
    {
        std::vector<SampleType> result = constant<SampleType>(count, 0.25);
        if (!result.empty())
        {
            result.front() = std::numeric_limits<SampleType>::infinity();
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> negativeInfinitySignal(const std::size_t count)
    {
        std::vector<SampleType> result = constant<SampleType>(count, 0.25);
        if (!result.empty())
        {
            result.front() = -std::numeric_limits<SampleType>::infinity();
        }
        return result;
    }

    template <typename SampleType>
    [[nodiscard]] std::vector<SampleType> denormalSignal(const std::size_t count)
    {
        return constant<SampleType>(count, static_cast<double>(std::numeric_limits<SampleType>::denorm_min()));
    }
}
