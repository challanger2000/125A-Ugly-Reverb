# 125A Ugly Reverb

A dark, metallic character reverb focused on clang, resonance, industrial spaces and early-digital character.

## Design goal

Ugly Reverb intentionally cultivates qualities that modern reverbs often try to suppress: audible modes, metallic ringing, clang, hard reflections and artificial spatial structure.

The target is not "bad reverb". The target is **controlled ugliness**: useful, repeatable and musical character for guitars, snares, synths, vocals, sound design and industrial/dark productions.

## Development V0.1

The first sound-probe build contains:

- Stereo VST3
- FDN-based algorithmic reverb core
- Material: Plate / Steel / Tank
- Size
- Decay
- Pre-Delay
- Diffusion
- Damping
- Metal
- Clang
- Rattle
- Body
- Width
- Mix
- Output
- Digital Color: Clean / 12-bit / 8-bit
- Bypass
- 32-bit float processing
- No intentional plug-in latency
- No custom GUI yet: first priority is validating the sound itself

### DSP principle

Metal, Clang and Rattle are not merely post-effects after a conventional reverb. They alter the feedback/diffusion behaviour of the reverb network itself.

### Current material modes

- **Plate** — dense but intentionally early-digital/metallic
- **Steel** — harder, more exposed modal structure
- **Tank** — larger resonant metallic body

The current branch is an early development sound probe, not a release.
