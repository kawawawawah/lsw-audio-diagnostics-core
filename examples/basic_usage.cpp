// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "lsw/audio_diag/analyzer.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr std::size_t blockSize = 128U;
    constexpr double frequency = 1000.0;
    constexpr double pi = 3.14159265358979323846;

    lsw::audio_diag::Analyzer<float> analyzer;
    lsw::audio_diag::AnalyzerConfig config {};
    config.sampleRate = sampleRate;
    config.maximumBlockSize = blockSize;
    config.numberOfChannels = 2U;
    config.levelTimeConstantSeconds = 0.001;
    config.correlationTimeConstantSeconds = 0.001;
    if (!lsw::audio_diag::isSuccess(analyzer.prepare(config)))
    {
        std::cerr << "Analyzer preparation failed.\n";
        return 1;
    }

    float left[blockSize] {};
    float right[blockSize] {};
    for (std::size_t sample = 0U; sample < blockSize; ++sample)
    {
        const double phase = (2.0 * pi * frequency * static_cast<double>(sample)) / sampleRate;
        left[sample] = static_cast<float>(std::sin(phase));
        right[sample] = left[sample];
    }
    const float* channels[] { left, right };
    analyzer.process(channels, 2U, blockSize);

    const lsw::audio_diag::Snapshot snapshot = analyzer.getSnapshot();
    std::cout << "Peak: " << snapshot.channels[0].samplePeak << '\n'
              << "Held Peak: " << snapshot.channels[0].heldPeak << '\n'
              << "RMS: " << snapshot.channels[0].smoothedRms << '\n'
              << "Correlation: " << snapshot.stereo.correlation << '\n'
              << "Clip Count: " << snapshot.channels[0].clipCount << '\n'
              << "Dropout Events: " << snapshot.channelEvents[0].dropout.eventCount << '\n'
              << "Sustained Clip Events: " << snapshot.channelEvents[0].sustainedClip.eventCount << '\n'
              << "DC Fault Events: " << snapshot.channelEvents[0].dcFault.eventCount << '\n'
              << "Invalid Burst Events: " << snapshot.channelEvents[0].invalidBurst.eventCount << '\n';
    return 0;
}
