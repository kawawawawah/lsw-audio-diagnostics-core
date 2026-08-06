// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#include "correlation_tracker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lsw::audio_diag::detail
{
    namespace
    {
        [[nodiscard]] double finiteProduct(const double left, const double right) noexcept
        {
            const double absoluteLeft = std::abs(left);
            const double absoluteRight = std::abs(right);
            const double maximum = std::numeric_limits<double>::max();
            if (absoluteLeft == 0.0 || absoluteRight == 0.0)
            {
                return 0.0;
            }
            if (absoluteLeft > maximum / absoluteRight)
            {
                return std::signbit(left) == std::signbit(right) ? maximum : -maximum;
            }
            return left * right;
        }

        [[nodiscard]] double finiteDifference(const double left, const double right) noexcept
        {
            const double maximum = std::numeric_limits<double>::max();
            const double absoluteLeft = std::abs(left);
            const double absoluteRight = std::abs(right);
            if (std::signbit(left) == std::signbit(right))
            {
                return std::abs(left - right);
            }
            return absoluteLeft > maximum - absoluteRight ? maximum : absoluteLeft + absoluteRight;
        }
    }

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
        smoothedLeftSquared_ = (alpha_ * smoothedLeftSquared_) + (blend * finiteProduct(left, left));
        smoothedRightSquared_ = (alpha_ * smoothedRightSquared_) + (blend * finiteProduct(right, right));
        smoothedProduct_ = (alpha_ * smoothedProduct_) + (blend * finiteProduct(left, right));
        maximumAbsoluteDifference_ = std::max(maximumAbsoluteDifference_, finiteDifference(left, right));
    }

    double CorrelationTracker::correlation() const noexcept
    {
        const double denominator = std::sqrt(smoothedLeftSquared_) * std::sqrt(smoothedRightSquared_);
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
        return (std::sqrt(smoothedLeftSquared_) * std::sqrt(smoothedRightSquared_)) > 1.0e-12;
    }
}
