# Design

The UI uses the DHN9 monochrome 8-bit system: 8 px grid, grayscale palette, procedural transfer-curve and blade-bar motif, no external images, and no external fonts. The editor default size is 960 x 544 and the minimum is 720 x 432.

Every parameter has a visible custom control attached to APVTS, plus a component ID, accessible name/title/description, tooltip, and keyboard focus. The editor is custom JUCE UI code.

The audio callback owns no file, network, logging, lock, or heap allocation work in steady state. `FoldKnifeDSP::prepare` sizes the substep scratch buffer and resets per-channel state; `processBlock` uses fixed channel pointers and bounded frame-major loops only.

The transfer-curve motif is rendered procedurally from the live fold, bias, and symmetry parameter values so the product identity remains visible without bundled assets.
