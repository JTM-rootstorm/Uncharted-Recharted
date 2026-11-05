# Findings

Dated notes on the game as SwARc's PS4 platform runs it.

## 2026-11-04: the dump and its imports

- CUSA02320, base game plus patch; the union the console mounts was
  mirrored over GoldHEN's FTP (15,146 files, 59 GB; sizes checked against
  the console's listings). GoldHEN decrypts the executables on read.
- Three executables: eboot.bin (the first game), big2-ps4_Shipping.elf
  and big3-ps4_Shipping.elf. All three import
  sceSystemServiceLoadExec, presumably how one game starts another (not
  measured yet).
- Shipped PRXs: libc and libSceFios2 only.
- orbis_app imports (SwARc ps4 515dc04): 594/637/634 imports, 97/125/121
  unhandled. New libraries against Gravity Rush: libSceVideodec2 (the
  hardware video decoder), libScePosix (sockets, files, pthreads; second
  and third games), NpSignaling/NpScore/NpTus, Mouse, ErrorDialog,
  NpProfileDialog, Remoteplay. libkernel: 36 missing (user events,
  module info and unwind queries, VirtualQuery, signals, setcontext).
  GnmDriver: 7 (embedded VS/PS shaders, the validation and trace
  queries).

## 2026-11-04: first run of the first game (eboot.bin)

- Lift: 25,333 functions, 1.88 M instructions, 417 unimplemented sites;
  as in Gravity Rush's, nearly all are data decoded as code. Real ones:
  vcvtph2ps (F16C, 18 sites) and one paddb; fatal only if reached.
- The memory setup (AllocateMemory in memory-area-ps4.cpp, per its assert strings)
  allocates 1600 MiB of direct memory and maps it with
  sceKernelMapDirectMemory, hint 0x1000000000, flags 0, prot 0x33,
  then asserts `vaddr == (void*)pageAddr` (int 0x41 at 0x861CD6): it
  relies on the hint being honoured. SwARc ps4 5f7fbe6 widened the
  arena to 256 GiB for it.
- Next stop: libkernel!sceKernelSetVirtualRangeName, called right after
  that mapping (return address 0x861D05).
