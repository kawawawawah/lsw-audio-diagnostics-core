// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include "lsw/audio_diag/analyzer_config.hpp"

namespace lsw::audio_diag::detail
{
    class CorrelationTracker
    {
    public:
        void configure(const AnalyzerConfig& config) noexcept;
        void reset() noexcept;
        void beginBlock() noexcept;
        void processPair(double left, double right) noexcept;
        [[nodiscard]] double correlation() const noexcept;
        [[nodiscard]] double maximumAbsoluteDifference() const noexcept;
        [[nodiscard]] bool isAvailable() const noexcept;

    private:
        double alpha_ = 0.0;
        double smoothedLeftSquared_ = 0.0;
        double smoothedRightSquared_ = 0.0;
        double smoothedProduct_ = 0.0;
        double maximumAbsoluteDifference_ = 0.0;
    };
}
