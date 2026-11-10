# Uncharted-Recharted

Uncharted: The Nathan Drake Collection (PS4, 2015) as a native Linux
program, through static recompilation with
[SwARc](https://github.com/JTM-rootstorm/SwARc): the game's x86-64
machine code is lifted to C++ ahead of time and runs on reimplemented
PS4 system libraries (SwARc's Orbis platform module). There is no
emulator in the result, and no game code or data in this repository:
bring your own decrypted copy of the game, dumped from your own console.

It is the second PS4 game on SwARc, after
[Gravity Rush](https://github.com/JTM-rootstorm/GravityRush-Reshifted).
Its job is the platform more than the game: what it needs that Gravity
Rush did not becomes part of the module, not of this repository.

## Status

Early. The first game (Uncharted: Drake's Fortune) starts, plays its
intro movies, reaches the title screen, starts a new game, saves, and
begins its opening cutscene, where it stops on an audio stream the
module does not handle yet. The second and third games are not run yet.
[FINDINGS.md](FINDINGS.md) has the detail, newest at the end.

## What you need

- The collection, CUSA02320, base game and patch, decrypted from your
  own console: the `app0` the console runs (the base and patch merged),
  and the game's `param.sfo`.
- Linux x86-64 with a Vulkan driver.
- A SwARc checkout next to this repository's parent (`../../SwARc`), or
  anywhere with `-DSWARC_DIR=...`. Its README lists what it needs to
  build (Clang, CMake 3.25+, Ninja, SDL3, ffmpeg, FreeType, fontconfig,
  glslang).

## Building

```sh
cmake --preset release
cmake --build --preset release
tools/lift.sh "$APP0"
cmake --preset release
cmake --build --preset release
```

The first build makes the lifter and the loader; `tools/lift.sh` then
lifts the game's executables and the libraries it ships into
`generated/` (`generated/lift_report.txt` lists what was left
unlifted), and the second build compiles them into `bin/ucr`. Lift
again after updating SwARc's lifter or the game.

## Running

```sh
out/build/release/bin/ucr --game-dir "$APP0" --param-sfo "$PARAM_SFO"
```

Save data goes in `$XDG_DATA_HOME/ucr` (else
`~/.local/share/ucr`). `UCR_TRACE=1` logs every system library call.
Anything the module has not implemented stops the game with a message
naming it, rather than guessing.
