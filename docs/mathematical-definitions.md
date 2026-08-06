# Mathematical definitions

For a valid sample `x`:

```text
samplePeak = max(abs(x)) within the most recent block
maximumAbsoluteSample = max(abs(x)) since reset
```

The unsmoothed RMS definition is:

```text
rms = sqrt(mean(x^2))
```

The published RMS is an exponentially smoothed RMS. For every valid or sanitized sample, the analyzer updates smoothed power and then takes its square root:

```text
power[n] = alpha * power[n - 1] + (1 - alpha) * x[n]^2
smoothedRms = sqrt(power[n])
```

`alpha = exp(-1 / (timeConstantSeconds * sampleRate))`. dBFS is `20 * log10(max(rms, 1e-8))`, clamped to -160 dBFS so silence never yields negative infinity.

DC offset uses the same exponential form with signed samples:

```text
dc[n] = alphaDc * dc[n - 1] + (1 - alphaDc) * x[n]
```

For stereo input, smoothed sums estimate:

```text
correlation = sum(L * R) / sqrt(sum(L^2) * sum(R^2))
balanceDb = leftRmsDb - rightRmsDb
monoCompatibilityScore = clamp((correlation + 1.0) * 0.5, 0.0, 1.0)
```

When the correlation denominator is extremely small, correlation is reported as 0.0 and no stereo relation flag is raised. The mono score is an intentionally simple diagnostic heuristic, not a standards-defined measurement.

NaN, infinity, and subnormal samples are counted then replaced with zero before all formulas above are evaluated.
