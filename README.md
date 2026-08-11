# FoldKnife

FoldKnife is an EHL Digital Harsh Noise distortion effect built around a 4x oversampled asymmetric wavefolder, hard-clip modes, explicit bias/symmetry controls, DC blocking, and a deliberate alias mode for exposed digital harshness.

## Identity

- Product: `FoldKnife`
- Repository slug: `foldknife`
- Bundle ID: `jp.ehl.foldknife`
- Manufacturer: `EsionHsrahLatigid`
- Manufacturer code: `EHL_`
- Plugin code: `FdKn`

## Parameters

- `drive`: nonlinear drive into the folding stage.
- `fold`: density of wavefold turning points.
- `clipMode`: folded, hard clip, or folded hard clip.
- `bias`: signed offset before the asymmetric transfer.
- `symmetry`: positive/negative fold balance.
- `preGain`: extra pre-stage gain in dB.
- `postTone`: post-folder tone contour.
- `aliasMode`: intentionally bypasses the oversampled path for DHN alias artifacts.
- `oversampleMode`: default 4x nonlinear processing path.
- `dcGuard`: high-pass DC blocker after asymmetric folding.
- `mix`: wet/dry blend.
- `output`: final output gain in dB.

## Build

```sh
cmake --preset engine-debug
cmake --build --preset engine-debug
ctest --preset engine-debug --output-on-failure

cmake --preset plugin-release -DEHL_JUCE_SOURCE_DIR=/path/to/JUCE
cmake --build --preset plugin-release --target ehl_stage_products
ctest --preset plugin-release --output-on-failure
```

The project pins JUCE to `91ad83ae34a81e0833b1a2b0866f54846370ae53` when network FetchContent is used. Set `EHL_JUCE_SOURCE_DIR` for offline/local builds.

Stable artifacts:

```text
artifacts/plugin-release/macos-arm64/standalone/foldknife_standalone_plugin.app
artifacts/plugin-release/macos-arm64/vst3/foldknife_vst3_plugin.vst3
artifacts/plugin-release/macos-arm64/au/foldknife_au_plugin.component
artifacts/plugin-release/macos-arm64/ARTIFACTS.txt

artifacts/plugin-release/windows-x64/standalone/foldknife_standalone_plugin.exe
artifacts/plugin-release/windows-x64/vst3/foldknife_vst3_plugin.vst3
artifacts/plugin-release/windows-x64/ARTIFACTS.txt
```

## Tests

Targets are fixed for CI and humans:

- `foldknife_dsp_tests`
- `foldknife_plugin_tests`
- `foldknife_editor_tests`
- `ehl_stage_products`

## Anti-Alias Note

The default mode reduces nonlinear aliasing by processing the wavefolder at 4x and averaging the shaped path, but it is not a mathematically perfect band-limited distortion model. `aliasMode` is deliberately exposed because Digital Harsh Noise sometimes wants audible foldover; it is a named parameter rather than an accidental implementation detail.

## References

- Julian D. Parker, Vadim Zavalishin, and Efflam Le Bivic, "Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution."
- Holters and Zolzer, "Antiderivative Antialiasing for Memoryless Nonlinearities."
- Esqueda, Välimäki, and Bilbao, "Rounding Corners With BLAMP."
- Kahles, Esqueda, and Välimäki work on aliasing-aware virtual analog distortion and antialiasing tradeoffs.
