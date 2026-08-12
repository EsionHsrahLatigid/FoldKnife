# Design

## UI

The editor uses the strict DHN9 simple monochrome 8-bit contract: flat four-level palette only (`#050505`, `#2A2A2A`, `#8A8A86`, `#F2F2F0`), 4 px base grid / 8 px major spacing, product name at `y=16`, compact function label at `y=48`, one 1 px divider at `y=72`, and controls starting at absolute `y=80`. There is no full-canvas grid, tagline, package ID, decorative motif, fake visualizer, meter, panel, outer frame, or parameter-driven atmospheric paint. The editor default size remains 960 x 544 and the minimum remains 720 x 432.

Every parameter keeps a visible custom control attached to APVTS, plus a component ID, accessible name/title/description, tooltip, and keyboard focus. The editor remains custom JUCE UI code. DSP, parameter IDs, bundle ID, and accessibility contracts are unchanged.

## DSP

The audio callback owns no file, network, logging, lock, or heap allocation work in steady state. `FoldKnifeDSP::prepare` sizes the substep scratch buffer and resets per-channel state; `processBlock` uses fixed channel pointers and bounded frame-major loops only.
