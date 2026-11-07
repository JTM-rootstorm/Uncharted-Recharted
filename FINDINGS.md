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

## 2026-11-05: the console's answers (hlevals runs 1-4)

Logs and the three decoded frames: PS4-HB/dumps/11.00/hlevals_plugin_ucr_run1-3.txt
and hlevals_ucr_run4/. Runs 1-3 fixed the plugin (libraries the game
loads with sceSysmoduleLoadModule are hooked after that call, at the
address their module exports; AppContent's functions live in the
library libSceAppContentUtil and are hooked at their 11.00 offsets).

- AppParamGetInt(1) without USER_DEFINED_PARAM_1: 0, value 0.
- PlayGo: handle 319 on the console; language mask 0x4000000000000000;
  Prefetch 0. GetProgress and SetLanguageMask are not called on the way
  into the first game.
- DownloadDataGetAvailableSpaceKb("/download0"), DOWNLOAD_DATA_SIZE 0:
  0x80D90004, the size not written.
- One system event pending at start: type 0x10000003, no data.
- Mouse with nothing attached: Init 0, Open -> 0x160500, Read -> 1.
- Gamma 1.0 -> settings {1, 1, 1, 0}; AdjustColor_ (size 16) 0.
- Trophies: 54 in the set, none unlocked (that user).
- NpTus (the console's user is signed in): title contexts 0x10000001 on,
  requests never finish (offline). SwARc models a user not signed in.
- Compute queues: pipe p queue 1 -> id 8p + 2; Videodec2's own queue
  pipe 4 queue 7 (16384 dwords) -> 0x28, its interrupt GPU event ident 4.
- Videodec2, for this game's config (H.264, 1920x1088, 4 reference
  frames, flags 0x100):
  - compute memory 0x495200; the queue handle the memory's base +
    0x290500;
  - decoder memory: CPU 0x334CC00, GPU 0x48EAE00, 0x304B000 and 0x3314
    (the last two the frame-buffer query's), alignment 0x100; the
    decoder handle is the CPU memory's base;
  - output: the first picture on the 8th access unit, then one a call,
    in display order, copied into the frame buffer given with that call
    (output info: valid, picture count 1, codec 1, 1920 / pitch 1920 /
    1088, the frame buffer's address and size). Flush hands out the
    other 7 one a call, then an empty one;
  - the picture: linear NV12, pitch 1920, 1088 rows of luma (the 8
    padding rows decoded, 16), interleaved chroma at 1920 * 1088.
    ffmpeg's decode of the same stream matches it byte for byte;
  - GetPictureInfo (first argument only, 104 bytes): pts and dts as
    given, attached data (the access unit's index), profile 100, level
    42, width/height in macroblocks - 1 (119, 67), frame_mbs_only,
    cropping (bottom 8 rows), no aspect ratio or video signal type (the
    defaults: format 5, colour 2/2/2), timing 1/120 not fixed,
    bitstream restriction, max_dec_frame_buffering 4.
  All 127 of the game's movies have that SPS.

## 2026-11-05: past the title's first frames

- The title's HDR target (1920x1088, 16_16_16_16 float, FAST_CLEAR on)
  is cleared by a compute dispatch that zeroes its whole CMASK (0x5000
  bytes); clear colour (0, 0, 0, 1) in CLEAR_WORD0/1. Modelled in SwARc
  (colour fast clears).
- Then image_gather4_c_o, a 64-bit address summed with s_add_u32 +
  s_addc_u32, and MXCSR.DAZ with denormal operands on the CPU: all
  handled now.
- The tiled lighting compute shader: its per-light records (0xE0 bytes
  from the user-data pointer, T#/S# at +0x90/+0xB0) and shadow T#s
  (0x70D0 + 32 * index) are indexed by a v_readfirstlane of the tile's
  light bitmask; handled now (run-time offsets in SwARc). At the title
  all light T#s are zero and the 128 shadow entries one 16x16 T#.
- That shader's wave-wide min/max depth reduction (ds_swizzle xor 1-16,
  then v_readlane of lanes 0 and 32) and its v_readlane / waterfall loops
  (v_readfirstlane into M0, v_cmpx, v_movrels) need real 64-lane waves:
  SwARc ps4 27f9eff runs such compute stages in waves. The same shader
  loads the light records per lane through ADDR64 buffer loads (a V# of
  base 0, no range; v[12:13] = s[0:1] + 0xE0 * light), handled there too.
- Then, in the order reached (SwARc ps4 aa3bd8b..): the stencil sampled
  as 8 UINT; a 3D texture in tile mode 19 (1D thick); a pixel shader's
  waterfall loop with v_movrels (VGPR indexed by M0);
  EVENT_WRITE of PIXEL_PIPE_STAT_CONTROL (00000138 0007FC00: counter 0,
  stride 2, render backends 0-7).
- Current stop: EVENT_WRITE of PIXEL_PIPE_STAT_DUMP (00000139 3857A6F0
  00000010: event 0x39 index 1 to 0x103857A6F0), an occlusion query.
  SwARc's host GPU layer has no occlusion queries (SDL_GPU has none and
  draws cannot write storage), and the dump's memory layout on 11.00 is
  not measured (presumably a 64-bit count per render backend, 16 bytes
  apart, begin and end, bit 63 valid). SwARc ps4 now counts the pixels
  of the draws DB_COUNT_CONTROL counts (one exact 64-bit counter); the
  dump still needs a console measurement of what it writes (the plugin
  reading the query buffer after the GPU has written it).

## 2026-11-06: the game's occlusion queries

- The packets come from the game's own Gnm (0x8737B0, begin or end of a
  query at a): for a begin, DMA_DATA zero-fills 128 bytes at a and
  WAIT_REG_MEM waits for it; then EVENT_WRITE 0x138 (0x7FC00) and
  EVENT_WRITE 0x139 to a (begin) or a + 8 (end). So a query is 128 bytes:
  8 render backends, 16 bytes apart, begin then end. A query pool
  (0x41CF50..0x41D0B0) hands out consecutive 128-byte queries, memsets
  the pool on reset, ends a frame's queries with an end-of-pipe label
  (0x873A40) and on completion sets each query's result flag to 1.
  What the GPU writes in those 16 bytes (count width, a valid bit,
  unused backends) is what the console run measures.
- The hlevals plugin now hooks sceGnmSubmitCommandBuffersForWorkload and
  SubmitAndFlipCommandBuffersForWorkload (11.00's plain Submit and
  SubmitAndFlip jump to them), finds the dumps in the command buffers and
  the indirect buffers they call, logs the first 8 submits that have any
  (the control word, the dump addresses, DB_COUNT_CONTROL), and a thread
  logs every change of the first 4 queries' 128 bytes for up to 2
  minutes. The dump is in the top-level command buffers of the main
  renderer's first submit (76 buffers in one sceGnmSubmitCommandBuffers;
  the plugin's first run scanned only 16 and saw none, log
  hlevals_plugin_ucr_run5.txt). Console run: upload hlevals.prx to /data/GoldHEN/plugins/
  (plugins.ini already has [CUSA02320]), start the first game and leave it
  at the title for 2 minutes, then fetch /data/hlevals/plugin.txt.
