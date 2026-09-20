# 125A Ugly Reverb

A dark, metallic character reverb focused on clang, resonance, industrial spaces and early-digital character.

## Design goal

Ugly Reverb intentionally cultivates qualities that modern reverbs often try to suppress: audible modes, metallic ringing, clang, hard reflections and artificial spatial structure.

The target is not "bad reverb". The target is **controlled ugliness**: useful, repeatable and musical character for guitars, snares, synths, vocals, sound design and industrial/dark productions.

## Development V0.1

The first sound-probe build contains:

- Stereo VST3
- old-digital comb/allpass character-reverb core
- Material: Plate / Thin Plate / Heavy Plate / Sheet / Spring / Steel / Pipe / Metal Drum / Oil Can / Chamber / Tank
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

- **Plate** — dense, early-digital metallic sheet
- **Thin Plate** — brighter, faster and lighter sheet character
- **Heavy Plate** — denser and weightier plate body
- **Sheet** — thin, direct, deliberately scheppernd metal
- **Spring** — mechanical, resonant and lightly moving spring character
- **Steel** — harder, more exposed modal structure
- **Pipe** — narrow, tubular and strongly modal
- **Metal Drum** — hollow metal shell / barrel-like body
- **Oil Can** — gently unstable electromechanical-style character
- **Chamber** — cold industrial resonant chamber
- **Tank** — larger resonant metallic body

The current branch is an early development sound probe, not a release.
