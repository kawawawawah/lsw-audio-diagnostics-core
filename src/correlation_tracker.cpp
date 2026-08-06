// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "correlation_tracker.hpp"

#include <algorithm>
#include <cmath>

namespace lsw::audio_diag::detail
{
    void CorrelationTracker::configure(const AnalyzerConfig& config) noexcept
    {
        alpha_ = std::exp(-1.0 / (config.correlationTimeConstantSeconds * config.sampleRate));
        reset();
    }

    void CorrelationTracker::reset() noexcept
    {
        smoothedLeftSquared_ = 0.0;
        smoothedRightSquared_ = 0.0;
        smoothedProduct_ = 0.0;
        maximumAbsoluteDifference_ = 0.0;
    }

    void CorrelationTracker::beginBlock() noexcept
    {
        maximumAbsoluteDifference_ = 0.0;
    }

    void CorrelationTracker::processPair(const double left, const double right) noexcept
    {
        const double blend = 1.0 - alpha_;
        smoothedLeftSquared_ = (alpha_ * smoothedLeftSquared_) + (blend * left * left);
        smoothedRightSquared_ = (alpha_ * smoothedRightSquared_) + (blend * right * right);
        smoothedProduct_ = (alpha_ * smoothedProduct_) + (blend * left * right);
        maximumAbsoluteDifference_ = std::max(maximumAbsoluteDifference_, std::abs(left - right));
    }

    double CorrelationTracker::correlation() const noexcept
    {
        const double denominator = std::sqrt(smoothedLeftSquared_ * smoothedRightSquared_);
        if (denominator <= 1.0e-12)
        {
            return 0.0;
        }
        return std::max(-1.0, std::min(1.0, smoothedProduct_ / denominator));
    }

    double CorrelationTracker::maximumAbsoluteDifference() const noexcept
    {
        return maximumAbsoluteDifference_;
    }

    bool CorrelationTracker::isAvailable() const noexcept
    {
        return std::sqrt(smoothedLeftSquared_ * smoothedRightSquared_) > 1.0e-12;
    }
}
