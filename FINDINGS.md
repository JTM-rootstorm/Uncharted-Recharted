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
- Then, handled in SwARc ps4 6c3a24b in the order reached:
  sceKernelSetVirtualRangeName, sceFiberOptParamInitialize,
  scePthreadAttrSetschedpolicy (policy 3, RR), sceVideoOutGetResolution-
  Status, sceGnmAddEqEvent ids 0 to 6 (beside 0x40: the game registers
  0x40 and 0-6 on separate queues, 0x922D2E), sceRemoteplayInitialize.
- Stop: sceAppContentAppParamGetInt(1). param.sfo has no
  USER_DEFINED_PARAM_1; what the system answers is not known. The
  GoldHEN plugin (PS4-HB/plugins/hlevals) now logs it from the game on
  the console, with the compute queues' pipes and the GPU events' idents.

## 2026-11-05: what the platform can fill without the console

- SwARc ps4 284100a adds the libkernel imports whose behaviour 11.00's
  libkernel defines on its own: user events (EVFILT_USER; level, or edge
  with EV_CLEAR; a trigger carries udata), the kevent accessors,
  scePthreadAttrGetdetachstate, sceKernelUtimes, __stack_chk_fail and
  libScePosix rename/unlink.
- sceAppContentAppParamGetInt is an IPMI call into a system service in
  11.00's libSceAppContent, so the stop stays until the console answers.
  The same holds for scePlayGoGetLanguageMask, GetProgress and Prefetch
  (the PlayGo service).
- The movies are raw H.264 Annex-B streams (x264): shareddata/movies/ps4/
  sce-ndi-logos.bsf (the logos, first), uNdata/build/main/movie1/ps4/
  *.bsf. They go through libSceVideodec2, a checking layer over
  libSceVdecCore. Its structure sizes and checks come from the
  library: decoder config and memory info 72 bytes, compute config
  16 and memory 24, Decode's input 48 (AU pointer and size, pts,
  dts, attached data), frame buffer 32 (256-aligned), output info 0x30
  or 0x38. The memory sizes, the output delay and the picture layout
  come from VdecCore and the driver: measured, not derived.
- The hlevals plugin now also logs the game's Videodec2 calls (the
  memory queries in and out, the first 48 Decode calls, GetPictureInfo)
  and PlayGo's GetProgress and Prefetch; plugins.ini has [CUSA02320].
  Console run: upload hlevals.prx and plugins.ini, enable the plugin
  loader, start the collection, let the logos play into the first
  game's title, then fetch /data/hlevals/plugin.txt.
