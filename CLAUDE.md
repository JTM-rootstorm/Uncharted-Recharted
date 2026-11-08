# Uncharted-Recharted: Uncharted: The Nathan Drake Collection (PS4, CUSA02320) → native Linux/macOS static recomp

Rules only. Status, findings and next steps go in FINDINGS.md. The
lifter, the runtime and the PS4 platform module (platforms/orbis) live
in the shared SwARc library (../../SwARc, its own git repo with its own
CLAUDE.md); this repo holds what is specific to this game. It is the
second PS4 user of SwARc, after ../GravityRush: its job is to move the
platform along, not this game alone.

## Layout
- Project: ~/Documents/Programming/Recomp/PS4/UnchartedRecharted
  (local git repo, no remote yet).
- Library: ~/Documents/Programming/Recomp/SwARc (add_subdirectory;
  SWARC_DIR overrides the path). Library changes are made in a
  topic worktree (../../SwARc-<topic>, cut from main) and landed with
  its tools/land.py; its rules apply.
- Game: the decrypted app0 (base game + patch, the union the console
  runs) dumped from the user's own console (NEVER commit or copy into
  the repo):
  ~/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA02320/mnt/sandbox/pfsmnt/CUSA02320-app0-patch0-union/
  (eboot.bin = the first game, big2-ps4_Shipping.elf and
  big3-ps4_Shipping.elf = the second and third, sce_module/,
  u1data/ to u3data/, shareddata/). param.sfo is at
  PS4-HB/dumps/games/CUSA02320/system_data/priv/appmeta/CUSA02320/param.sfo.
- Firmware names for NIDs (PS4-HB/inventory, PS4-HB/refs) are local
  references only, never copied here or into the library.

## Build
- `cmake --preset release && cmake --build --preset release`
- Lifting is a separate step (needs a build first, it runs orbis_app);
  re-run it after any change to the library's lifter or the binaries,
  then re-run cmake (the generated file list is globbed):
  `tools/lift.sh "$HOME/Documents/Programming/Recomp/PS4-HB/dumps/games/CUSA02320/mnt/sandbox/pfsmnt/CUSA02320-app0-patch0-union"`
  generated/lift_report.txt lists unimplemented instructions and
  warnings.
- Run: `out/build/release/bin/ucr --game-dir <app0> --param-sfo <param.sfo>`.
  UCR_TRACE=1 logs every HLE call; UCR_BACKGROUND=1 for automated runs,
  which go inside a memory cap with dummy audio and SDL asserts that
  abort:
  `systemd-run --user --scope --quiet -p MemoryMax=6G -p MemorySwapMax=0 env UCR_BACKGROUND=1 SDL_ASSERT=abort SDL_AUDIO_DRIVER=dummy <command>`.

## Rules
- Never commit game files, generated code or anything derived from the
  binaries (.gitignore covers them).
- Unimplemented paths are fatal and say so, never a guess; every import
  gets an explicit handler, in SwARc, not here.
- Commits are GPG-signed; no co-author trailers or generated-by markers.
  Don't push without asking.
