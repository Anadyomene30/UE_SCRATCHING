# scratchvj

A scratchable video DJ instrument for turntablists, driven by DVS timecode.

Load any video onto a deck, scratch it with the platters, mix on the crossfader,
queue clips, drive every parameter from the mixer's knobs and buttons, play and
scratch **360 video** while steering the view with a knob, and run a rack of
effects in which **every audio effect is wired to its visual counterpart on the
same control**. Unreal Engine is a *client* of this application, not its core.

Hardware this is built around: **MWM Phase**, **Reloop Elite**, **Reloop RP-8000
MK2** — though nothing about the Elite is hard-coded (see *MIDI learn* below).

## Why this works at all

The MWM Phase does not emit proprietary data: its dock generates **standard DVS
timecode** on RCA. So no vendor SDK is needed — a timecode decoder yields absolute
position and signed velocity, which is exactly the raw material required.

Two design principles run through everything:

1. **Anything scratchable is a *function* of position, never an integrator.** A
   simulation moves forward and cannot reverse; a stream parameterised by `t`
   scratches perfectly. Hence the video engine indexes frames rather than playing a
   stream, and the audio engine is driven by position rather than by rate.

2. **Audio acts on the temporal frequencies of a 1D signal; video acts on the
   spatial frequencies of a 2D one.** Where the correspondence is exact it is
   implemented as such — a low-pass filter really is a blur, a bitcrusher really is
   posterisation. Where it is not, the nearest perceptual analogue is chosen and
   documented. See [docs/fx-correspondances.md](docs/fx-correspondances.md).

## Try it

There is something to run, and it needs no hardware:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build
./build/scratchvj/scratchvj demo
```

A scripted performance drives the whole engine while a terminal dashboard shows
what it is doing — deck readouts, the filmstrip with its VRAM window, the surface
with untouched controls drawn as unknown, and the mapping engine's live output.
The script deliberately includes the awkward moments: a backspin, a loop scratched
inside, and a Phase link dropout, because those are the ones worth watching.

```
scratchvj demo [--seconds N] [--fps N] [--plain] [--record FILE]
scratchvj play FILE          replay a recorded take
scratchvj info FILE.svcache  what an analysed clip contains
scratchvj layout             the controls --midi-learn will ask you to sweep
```

`--record` writes a `.scratchtake`: the timestamped control stream of a
performance. That is how the rest of the project gets developed before the
turntables are plugged in — record once, then exercise every later change against
a real performance instead of a guess.

## Current state

The engine's logic is written and covered by **191 tests**; the parts that touch
hardware are not.

| Module | What it does |
|---|---|
| `core/midi` | MIDI stream decoding: running status, realtime interleave, 14-bit CC pairs |
| `core/surface` | The mirror of the mixer — one entry per control, each with a `known` flag |
| `core/layout` | The default Elite + RP-8000 control checklist |
| `core/learn` | MIDI learn: binds a control only after it proves it is really moving |
| `core/curve` | Range, deadzone, curve, inversion and smoothing |
| `core/mapping` | Routes any source to any destination through its own transform |
| `core/gestures` | Scratch rate, acceleration, backspin — and freezing on lost lock |
| `core/timecode` | Position tracking, the vinyl/wireless split, ABS/REL/INT transport |
| `core/anchor` | Follower mode: lining a clip up with Serato, and how stale that is |
| `core/transport` | Loops, hot cues, beat jump, slip |
| `core/videocache` | The `.svcache` clip format: fixed-size block-compressed frames |
| `core/framewindow` | The budget-driven rolling window of frames in video memory |
| `core/take` | Recording and replaying a performance's control stream |
| `core/protocol` | The UDP wire format carrying surface state to Unreal |
| `config/mapping_io` | `mapping.json`, written with names rather than numbers |
| `app/` | The simulation and the terminal dashboard |

Not yet written, and all of it needs hardware or heavy dependencies to be worth
writing: real MIDI and audio devices, the xwax timecode decoder, the FFmpeg
analysis pass, GPU rendering, the effect racks, the ImGui interface, the outputs,
and the Unreal plugin.

## Two details worth knowing up front

**Knobs are absolute potentiometers.** At launch the application genuinely does
not know where they are, and it will not invent a value: every control carries a
`known` flag and unknown controls are drawn as ghosts until first moved. This is
modelled rather than hidden because pretending otherwise would put wrong values on
screen and wrong values into Unreal.

**Nothing is hard-coded to the Elite.** Its CC map is not publicly documented, so
`--midi-learn` discovers it by asking the user to sweep each control. A knob binds
only after emitting several *distinct* values, so a neighbouring control brushed in
passing cannot steal the binding. The useful side effect is that the project works
with any other mixer.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires a C++20 compiler and CMake 3.20. CI builds and tests on Linux, macOS and
Windows on every push — the core carries no external dependencies specifically so
that portability is verified continuously rather than discovered late.

## Licence

**GPL-3.0.** The timecode decoder this project will build on (`timecoder.c` from
[xwax](https://xwax.org/)) is GPL-3, so `scratchvj` is too. The licence is declared
now rather than at the moment that code lands, because relicensing later would
require the agreement of every contributor by then.

The Unreal plugin is deliberately a **separate process boundary** — it only reads a
UDP socket and a shared texture — so it is not a derived work and may carry
whatever licence its author prefers. That boundary is architectural and legal at
the same time.
