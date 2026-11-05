// ndc: Uncharted: The Nathan Drake Collection (PS4), natively.
//
//   ndc --game-dir APP0 [--param-sfo FILE] [-- GAME ARGS...]
//
// APP0 is the game's app0 (eboot.bin, big2/big3-ps4_Shipping.elf,
// sce_module/, u1data/ to u3data/, shareddata/); SwARc's PS4 platform
// starts the eboot with the PRXs it ships and needs at start, at the
// bases tools/lift.sh lifted them at.

#include "orbis/process.h"

int main(int argc, char** argv) {
  recomp::GameConfig game;
  game.id = "ndc";
  return recomp::orbis::Main(game, argc, argv);
}
