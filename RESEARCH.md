# FoldKnife Research Notes

FoldKnife follows the DHN9 G001 distortion decision: a wavefolder should provide a sharp folded transfer identity, hard clipping must be available as a distinct mode, and aliasing should be controlled by default but deliberately exposable.

## Source Mapping

- Wavefolding and memoryless nonlinear distortion are established virtual-analog building blocks. The implementation uses a bounded triangular folding transfer plus hard clipping so every finite input maps to a finite output.
- Kahles, Esqueda, and Välimäki document why nonlinear distortion can create strong alias products and why antialiasing strategy has to be explicit rather than hidden behind broad "digital" wording.
- Esqueda/Välimäki/Bilbao BLAMP and related antiderivative antialiasing work show stronger approaches than this local implementation. FoldKnife does not claim perfect band limiting.
- JUCE provides the plugin, APVTS, UI, and build infrastructure. The DSP core stays dependency-light and preallocates its local buffers in `prepare`.

## Product Interpretation

Default processing uses four interpolated nonlinear substeps per sample, then averages the shaped substeps. This is an intentionally lightweight integration strategy, not filtered oversampling. The `aliasMode` parameter intentionally routes to the single-rate shaper for Digital Harsh Noise material where aliasing is part of the sound. The UI and documentation name this mode directly.

## Safety Invariants

- Non-finite input is sanitized.
- Parameters are range-clamped and smoothed inside the DSP core.
- Substep scratch memory is allocated in `prepare`, not in the audio callback.
- The nonlinear transfer, DC blocker, tone stage, wet/dry blend, and output gain are bounded.
- Matched mono and stereo layouts are supported; mono-to-stereo and stereo-to-mono layout changes are rejected.
