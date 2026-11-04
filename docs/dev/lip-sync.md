# Lip sync

How talking characters' mouths are animated, and why they stayed still on gcn-recomp
before the fork's commit 567ece9 (issue #26). This document holds names, addresses, sizes
and descriptions only. It contains no game code and no disassembly.

## How the game animates mouths

The mouth is driven by how loud the voice is, measured on the AX library's **AuxA** effects
bus. It is not driven by any data stored with the voice files.

- `fn_800C3E38(obj)` installs the game's own AuxA callback, `fn_800C3E7C`, through
  `AXRegisterAuxACallback` (`fn_80146830`). It also calls NU Sound's `fn_80189FBC` (with
  1, 0x40 and a float), which looks like it sets the voices' send level to that bus.
  Two talk-setup paths call it, at 0x800A9A10 and 0x800AB754.
- Every 5 ms audio frame, `fn_800C3E7C` copies the first 0x140 bytes (80 samples, 32-bit)
  of the AuxA left buffer that the DSP uploaded into a global at 0x802D2590. It leaves the
  buffer unchanged, so the bus's contents still mix back into the main output.
- `fn_800C3D54(obj)` turns that copy into a level. It averages the absolute values of the
  80 samples and divides by a per-object scale at `obj+0x38`, which `fn_800C3E30` sets.
  The face code calls it at 0x800AA968, 0x800AAAF4, 0x800AABC4, 0x800AAC94, 0x800ADA80,
  0x800ADB40 and 0x800B3ED4. Both the 3D briefing portrait and the 2D comm-box portraits
  go through this code.

So the mouth moves only if the voice reaches AuxA.

## Why it broke

The sound library is Namco's "NU Sound", built on the AX library of the April 2004 SDK
("<< Dolphin SDK - AX release build: Apr 5 2004 >>"). NU fills in each voice's mixer
control itself (around 0x80192A68–0x80192BA4). It uses the post-2001 form, with one bit for
each output:

| Bit | Output | Bit | Output |
| --- | --- | --- | --- |
| 0x0001 | main L | 0x0200 | AuxB L |
| 0x0002 | main R | 0x0400 | AuxB R |
| 0x0004 | surround | 0x1000 | AuxB S |
| 0x0010 | AuxA L | 0x4000 | Dolby Pro Logic II |
| 0x0020 | AuxA R | | |
| 0x0080 | AuxA S | | |

The bits it never sets, 0x8, 0x40, 0x100, 0x800 and 0x2000, are the volume-ramp bits for
each group of outputs. Dolphin's AX HLE reads the word this way for every ucode except the
2001 build (fingerprint 4E8A8B21).

gcn-recomp's AX HLE was written against that 2001 build, which Metroid Prime uses. In that
build bit 0x10 means "Dolby Pro Logic II mixing" and the low bits mean something else.
For a voice sent to main L/R and AuxA L/R (0x33), the HLE therefore mixed neither AuxA nor
AuxB. The voices played on the main bus, but AuxA stayed silent. The level meter read zero,
so the mouth stayed shut.

Two other ways the game might read a voice back were ruled out:

- AX syncs only a voice's state, current volume and current address from the DSP back to
  the CPU's copy of the voice. The HLE writes all of these. NU Sound's stream code uses
  the address to count how far a stream has played. Neither NU nor the game reads a voice's
  depop or ADPCM history fields.
- Outside the AX library, nothing indexes the DSP's parameter-block array.

## The fix and what else it changes

The fork's 567ece9 passes the ucode fingerprint to `ax_init()`. It keeps the compact 2001
reading for 4E8A8B21 and decodes every other build one bit per output. The `--log=dsp` boot
line says which form was picked.

Fixing the AuxA and AuxB sends affects more than mouths. Anything this game sends to those
buses was silent before. That includes NU's effects: "FX1" is reverb (`reverbStd`,
`reverbHi`) and "FX2" is chorus or delay. Those effects should now be audible, as on the
console. During transmissions the voice also comes back from AuxA into the main mix, so
voices may sound a little louder or fuller than before.

The Dolby Pro Logic II sound option hasn't been looked at. The HLE mixes the outputs that
a DPL2 voice names, the same way Dolphin does.
