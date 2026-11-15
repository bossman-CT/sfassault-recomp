# Audio quality

What the port does to the game's sound between the voices and the speakers, what this game
asks of the DSP, and how the changes for issue #47 were measured. Names, numbers and
descriptions only: no game code, no disassembly, no audio. Captures stay outside the repo.

## What the game asks for

The game's sound library (Namco's NU Sound, on the April 2004 SDK's AX) sets every voice to
**linear** sample-rate conversion. With `GCN_AXSTATS` the fork now counts, every 2 s, the
voice-milliseconds per conversion and the voice-frames with the per-voice low-pass filter on.
Over the whole `fortuna` route:

| Conversion | Voice-milliseconds |
| --- | --- |
| Polyphase, 8 / 12 / 16 kHz tables | 0 / 0 / 0 |
| Linear | 4,609,374 |
| None | 0 |
| Low-pass filter on | 0 voice-frames (its four PB words stay zero) |

Pitch ratios run from 0.5 (a 16 kHz sound played at 32 kHz) to about 1.38. So the console
itself interpolates this game's voices linearly. The fork's polyphase filter and low-pass
filter are right for games that use them, but they change nothing here: the 32 kHz mix
(`GCN_AIDWAV`) of the old build and the new one is byte-identical over 13 minutes of the route.

## Where the port was worse than the console: 32 to 48 kHz

The console's DAC takes the 32 kHz mix directly. The port has to bring it to the device's
48 kHz, and did that by linear interpolation, which the console never does. That step was
the clarity problem the port added. It is now a 64-tap windowed sinc (`gcn-recomp`
`runtime/audio.cpp`, `runtime/sinc.h`), on by default with no option.
`GCN_AUDIO_RESAMPLE=linear` brings back the old step, for comparing by ear.

Test tones through the two steps, at -6 dBFS (dB re the input; spur is the strongest other
component, dBc):

| Tone | Linear: level | Linear: spur (below 16 kHz) | Sinc: level | Sinc: spur |
| --- | --- | --- | --- | --- |
| 1 kHz | -0.02 | -60 (-60) | 0.00 | -97 |
| 4 kHz | -0.40 | -35 (-37) | 0.00 | -97 |
| 10 kHz | -2.57 | -16 (-22) | 0.00 | -97 |
| 12 kHz | -3.77 | -12 (-20) | 0.00 | -103 |
| 14 kHz | -5.25 | -7 (-18) | -0.01 | -97 |
| 15 kHz | -6.10 | -5 (-16) | -3.56 | -94 |

The linear step's worst spurs are images at 32 kHz minus the tone (20-22 kHz), and it also
folds aliases into the audible band (a 10 kHz tone gets one at 6 kHz). On the game's own
title-screen music (`title-short`, 35 s, captured at the device with `GCN_WAV`, against the
32 kHz mix it was made from):

| Band | Linear vs the 32 kHz mix | Sinc vs the 32 kHz mix |
| --- | --- | --- |
| 6-8 kHz | -1.5 dB | 0.0 dB |
| 8-10 kHz | -2.3 dB | 0.0 dB |
| 10-12 kHz | -3.5 dB | 0.0 dB |
| 12-13 kHz | -4.7 dB | -0.1 dB |
| 13-14 kHz | -5.6 dB | 0.0 dB |
| 14-15 kHz | -6.5 dB | -0.7 dB |
| 16-24 kHz (nothing real) | 9-14 dB of image energy | 25-30 dB less |

## Audio > Quality > Enhanced

Because the console interpolates every voice linearly, the console's own sound has the same
weaknesses at the voice stage: a 16 kHz sound played at 32 kHz keeps images of itself above
8 kHz only a few dB down, and pitched-up sounds alias. Enhanced (off by default) resamples
the voices with a 32-tap windowed sinc instead, for what the speakers play only. The game
never sees it: its aux buses (which drive the talking portraits' mouths, see
[lip-sync.md](lip-sync.md)) and the mix in guest memory stay the console's, and the AI DMA
plays the enhanced copy in their place. Replays and netplay are unaffected by the setting.

Over the whole `fortuna` route (the 32 kHz mix as played, `GCN_AIDWAV`), Enhanced against
Console:

| Band | Enhanced vs Console |
| --- | --- |
| below 2 kHz | 0.0 dB |
| 2-4 kHz | +0.5 dB |
| 4-6 kHz | +1.8 dB |
| 6-8 kHz | +2.2 dB |
| 8-10 kHz | -2.1 dB |
| 10-12 kHz | -1.7 dB |
| 12-14 kHz | -1.1 to -1.8 dB |
| 14-16 kHz | -3.6 to -5.4 dB |

Many of the game's sounds are recorded at 16 kHz and played at about 32. Below 8 kHz,
Enhanced keeps the top of what they hold, which linear interpolation dulls. Above 8 kHz,
where such a sound holds nothing, it removes most of the images linear interpolation leaves
there. Some of the 14-16 kHz drop is real content of full-rate sounds above the sinc's
cutoff (0.9 of Nyquist). The most changed stretches are the mission briefing (about 1:00
into the route, 10-16 kHz down 4.5 dB) and the radio chatter in the space battle (about
3:40, down 5 dB).

The setting is safe for replays and netplay because nothing the guest can read changes:
`tools\regress.ps1 -Set audio.quality=enhanced` reports the same frames as Console, and
`GCN_STATE_HASH` gives the same digest of RAM and ARAM at every frame checked (see the
pull request for the run).

Cost, on this PC: the output step takes 82 ms of CPU per minute of audio (0.14% of a core),
and Enhanced 31 ms per minute per voice it resamples.

## Listening test

Use headphones if you can: the differences are in the top octaves. Run from the project
folder with a build of this branch.

1. **The output step (always on now).** Start the game and let it sit on the title screen
   music for 20 seconds. Then quit and start it again with the old step:

   ```powershell
   $env:GCN_AUDIO_RESAMPLE = 'linear'; .\build\sfassault.exe
   ```

   (Clear it afterwards with `Remove-Item Env:GCN_AUDIO_RESAMPLE`.) The old step should
   sound duller, with less air on the hi-hats and the top of the strings (3-6 dB less
   above 10 kHz), and with a faint gritty haze on bright sounds. The new one is closer to
   what a console's DAC plays.

2. **Enhanced (optional, off by default).** Replay the `fortuna` route at normal speed:

   ```powershell
   .\build\sfassault.exe --replay=saves\routes\fortuna
   ```

   Listen to the mission briefing at about 1:00 (Falco and Fox talking over the
   briefing screen) and the space battle at about 2:00 to 3:40 (Peppy and an ape soldier on
   the radio, lasers, explosions). Then run it again with
   `--set=audio.quality=enhanced` added, or switch Audio > Quality in the menu (F1) and
   listen again. Enhanced should give the voices cleaner "s" and "t" sounds without a fizzy
   edge, and slightly fuller lasers and explosions in the 4-8 kHz range. Console is how the
   game sounds on a GameCube.
