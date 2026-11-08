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

## 2026-11-06: the console's occlusion dumps (hlevals run 6)

Log: PS4-HB/dumps/11.00/hlevals_plugin_ucr_run6.txt (run 5 saw no
dumps: the plugin scanned only 16 of the submit's buffers).

- Every main-renderer submit (114 to 118 command buffers) has one query:
  PIXEL_PIPE_STAT_CONTROL 0x7FC00 twice, a begin and an end dump
  (0x103857A6F0 and +8 in the first; later queries 0x90 lower each
  frame), DB_COUNT_CONTROL 0x11000102 (PERFECT_ZPASS_COUNTS, ZPASS
  enabled, both slice masks) then 0x11000100.
- The query's 128 bytes, zeroed first, then: 8 render backends, 16 bytes
  apart, each a begin and an end qword. Every one has bit 63 set over a
  running per-backend count (e.g. 0x800000208C078569; the backends
  differ, and the counts grow between frames). At the title, begin and
  end are equal in every backend: no pixel passes.
- SwARc ps4 2ea9382 writes the dumps that way (backend 0 holds the count,
  the others stay at 0).
- Current stop: s_not_b64 in a draw shader (11103B2AA4: BEEA086A).

## 2026-11-08: past s_not_b64; a GPU hang in a pixel shader's waterfall loop

- The shader at the old stop is an alpha test: v_cmp, s_and_b64 vcc,
  s_not_b64 vcc, vcc, s_and_saveexec_b64, then an export or a discard.
- Once it was translated, the first frame hung the GPU (NVRM Xid 109,
  CTX SWITCH TIMEOUT, then VK_ERROR_DEVICE_LOST in vkQueueSubmit). The
  hang froze the host's desktop each time, so later reproductions ran on
  lavapipe (VK_DRIVER_FILES=lvp_icd). Lavapipe's subgroups are too
  narrow for the game's wave compute shaders (the "host subgroups unlike
  its waves" fatal), so those dispatches were skipped while bisecting.
  Skipping them on the real GPU did not stop the hang, and neither did
  skipping the occlusion-counted draws.
- The culprit was a pixel shader (vertex shader + pixel shader pair
  drawn as a rectangle list) that reads VGPRs by a per-pixel index:
  s_mov_b64 s[10:11], exec, then a loop of v_readfirstlane s14, v0;
  s_mov_b32 m0, s14; v_cmpx_eq_u32 s[14:15], s14, v0; s_andn2_b64
  s[10:11], s[10:11], s[14:15]; v_movrels x2; s_mov_b64 exec, s[10:11];
  s_cbranch_scc1 back. The shader opens with s_wqm_b64 exec, exec, and
  SwARc spread that over the wave's quads (EXEC 0xF in a stage that runs
  one lane per invocation), so the loop cleared bit 0 and never the
  rest. SwARc ps4 9e8b858 keeps the WQM result to the invocation's lane.
- Current stop: DMA_DATA C4104000 00000004 (not implemented yet). Seen on
  lavapipe with the wave compute dispatches skipped; that stop is in the
  command processor, before any GPU work.

## 2026-11-08: GDS, s_nand_b64, the memory query and a contiguous view

All on lavapipe with the wave compute dispatches skipped (a local,
uncommitted change); the real GPU has not run the game since the hang
fix.
- DMA_DATA C4104000: data (4 bytes) into GDS. SwARc ps4 6207eeb models
  GDS for DMA_DATA; no shader of the game's has used GDS so far (DS
  instructions with the GDS bit are fatal).
- s_nand_b64 at 0x11103DCD08 (a draw's shader): 6207eeb.
- sceKernelVirtualQuery (0x8621CC) and what follows (the loop at
  0x862170): the game takes a 1 MiB block from its heap in the big
  direct mapping at 0x1000000000, queries it (asserting isDirectMemory
  and 1 MiB alignment of the physical address, start - addr + offset),
  munmaps it there and maps its offset again (flags 0x90, prot 0x33) at
  the next slot of a contiguous range: a virtually contiguous view of
  scattered physical blocks. SwARc ps4 009d40f answers the query from
  recorded mappings and keeps an unmapped range's contents for its next
  mapping.
- Current stop: libSceVideodec2!sceVideodec2QueryComputeMemoryInfo
  (return address 0x8A35F8): the hardware video decoder, not modelled
  yet.

## 2026-11-08: v_mbcnt, GDS ds_append, 64-bit LDS; into Videodec2

The game now runs on the real GPU again (no Xid); lavapipe cannot run
its wave compute shaders. SwARc ps4 42b4e24: v_mbcnt_lo/hi, ds_append on
the GDS (the CP's GDS bound read-write to compute stages in waves),
ds_write_b64/ds_read_b64.

Videodec2 (the movie path): the console run with the hlevals plugin
(ucr_plugin3.txt in the session scratchpad, not kept in the repo) logged
every call for the game's one decoder:
- QueryComputeMemoryInfo: 0x495200 (VdecCore: 4120 bytes of shader state
  rounded to 256, plus 4800768). AllocateComputeQueue: pipe 4, queue 7,
  checkMemoryType 1, memory at 0x1500000000; queue handle = memory +
  0x290500 (VdecCore's object past the shader state and ring).
- Decoder config: AVC (1/1), profile 100, level 42, 1920x1088, max DPB
  -1, pipeline depth 4, queue 0x1500290500, affinity 0x3F, priority -1,
  checkMemoryType 1. QueryDecoderMemoryInfo: cpu 0x334CC00, gpu
  0x48EAE00, cpuGpu 0x304B000, maxFrameBufferSize 0x331400, alignment
  0x100. CreateDecoder with cpu 0x1300000000, gpu 0x1400000000, cpuGpu
  0x1500600000.
- Decode: AUs of 1151, 71, 68... bytes, pts/dts -1; frame buffer 0x400000
  bytes. The first output came with the 8th AU (index 7), one per AU
  after; the output frame is copied into the frame buffer passed to
  that Decode (isAccepted set). Output: valid, codec 1, width 1920,
  pitch 1920, height 1088, the frame buffer and its size. Picture info
  (0x68 bytes, AVC): SPS/VUI facts (profile, level, size in MBs - 1,
  crop bottom 8, VUI timing 1/120) at the frame buffer's last KiB.
- Flush returns the remaining frames one per call.
- The frames the plugin dumped (PS4-HB/dumps/11.00/hlevals_ucr_run4,
  pictures 0, 30, 90 of shareddata/movies/ps4/sce-ndi-logos.bsf, a raw
  x264 Annex B stream) are NV12 at pitch 1920, chroma at 1920 x 1088,
  byte for byte ffmpeg's decode. The attached data the game passes is
  the access unit's index; pictures come out in display order (0, 3, 2,
  4, 1, 7, 6, 8, 5, ...: the stream's B-pyramid). The buffer's last KiB
  holds the decoder's picture record (GetPictureInfo's source).
- SwARc ps4 5f7a0fb implements Videodec2 on ffmpeg with those
  measurements; our run writes its pictures at the console's addresses
  (0x13039CCD00, ...). libSceNpTus: the game deletes a title context it
  never made (signed out): 0x8055070E, as 11.00's library answers.
- Current stop: libSceAjm, instance 0x4001 (ATRAC9, flags 0x100000041,
  thread 0114): a batch with one RunSplitBuffer job (flags
  0x800000001001: STREAM, MULTIPLE_FRAMES, rev 1) and no initialisation
  job first. What the ACP answers then is not measured: the hlevals
  plugin needs AJM hooks (instance flags, the batch, the sideband after
  BatchWait) and a console run.

## 2026-11-08: the full-call console run; AJM decodes a RIFF stream unasked

- PS4-HB's hlevals plugin now logs every call the game makes into the
  firmware (all 384 import slots of eboot.bin, libc.prx and
  libSceFios2.prx: arguments, memory before and after, results; AJM
  batches whole, with each job's sideband and output when BatchWait
  returns). The run: PS4-HB/dumps/11.00/hlevals_ucr_run6/plugin.txt
  (70 MB; all slots hooked, no failure). PS4-HB/plugins/hlevals/tools/
  ajm_jobs.py tabulates its AJM jobs. Check it before asking for
  another console run.
- The current stop, measured: the first AJM instance the game creates
  is 0x4007 on the console (codec 1 = ATRAC9, flags 0x100000041; then
  0x4009-0x400C, likewise never initialised; ours numbers from 0x4001).
  Its first RunSplitBuffer job (flags 0x800000001001) gets
  shareddata/sound/sce-ndi-logos.xvag from offset 0x310: a RIFF/WAVE
  file (fmt extensible, ATRAC9 GUID, config FE 7A 17 F0: 4 channels,
  48 kHz, superframe 768 bytes = 4 frames of 256 samples; fact: 695654
  samples, delay 256; data at +100, 680 superframes). With no
  initialisation job the ACP takes the configuration from that header
  and decodes. The sideband (result, internal result, consumed,
  written, total samples):
    job 1: 0x10 (output full, input left), 0, 2472, 0x6000, 3072
      = the 100-byte header, superframes 0-2 and frame 0 of superframe
      3 (68 bytes): 13 frames, the first 256 samples dropped (the fact
      delay), 3072 samples in 0x6000 bytes (4 x s16)
    job 2: 0x10, 0, 2286, 0x6000, 6144
    job 3: 0x10, 0, 856, 0x3000, 7680; job 4 on: one frame a job
      (output room 0x800), consumed 630, 50, 44, 44, 630, ...
  So a frame consumes its own length (byte-aligned), not a quarter of
  the superframe; the last frame of a superframe takes the rest. The
  first outputs are silence; 2635 of the instance's 2900 logged
  outputs are not.
- ffmpeg 8.1.3's ATRAC9 decoder rejects 45 of this stream's 680
  superframes ("Invalid block flags", "Invalid stereo band"), which the
  console decodes, so the HLE cannot decode it through ffmpeg. LibAtrac9
  (MIT, reverse-engineered from Sony's decoder) decodes one frame at a
  time and reports the bytes it used: the candidate to compare against
  the console's consumed counts and output bytes (the log has the first
  64 bytes of every job's output, and the whole output's FNV-1a).

## 2026-11-08: LibAtrac9 against the console

- LibAtrac9 (MIT, PS4-HB/refs/LibAtrac9) decodes all 680 superframes of
  the logos stream without an error. Its per-frame byte counts, with
  the last frame of each superframe taking the superframe's rest,
  reproduce the console's consumed count for every one of instance
  0x4007's 2689 jobs on that stream, the 100-byte header included.
- The last job (seq 13775): result 0x8, consumed 674 (44 + the last
  superframe's rest, 630), written 0x330 = 102 samples, total 695654:
  the output stops at the fact sample count, and 0x8 marks the stream's
  end. Result 0x10 = the output is full and input is left.
- PCM: of the 86080 samples the log holds (each job's first 32, read at
  the first BatchWait returning 0 for it; a wait returning 0x8093000F
  dumps the buffer before the job ran), 77872 (90.5%) match LibAtrac9
  rounded to nearest and the other 8208 differ by exactly 1; 291 whole
  output buffers are equal. The misses sit where the fraction is near
  .5 (error rms 0.29 LSB, mean +0.015); a float build of LibAtrac9, and
  truncating or flooring, fit no better. So the console's decoder
  computes with other arithmetic (the ACP is a DSP); the format, frame
  lengths and scaling are the same.
- Every instance the game never initialises starts on a RIFF header:
  0x4007 (twice: the logos stream at seq 4209, a 3.9 MB stream at
  14977, consumed 0xA27 on its first job),
  0x4009 (seq 13123; 4 channels), 0x400A, 0x400B, 0x400C (13662,
  13664, 14857; 1 channel, 256-byte superframes, all 48 kHz). A RIFF
  header on a running instance restarts it.
- Tools: PS4-HB/plugins/hlevals/tools/at9ref.c (LibAtrac9 frame by
  frame: bytes used per frame, s16 out) and ajm_pcm.py (a decode
  against the log's consumed counts and output rows).

## 2026-11-08: past the AJM stop (SwARc: ATRAC9 on LibAtrac9)

- SwARc now models the measured AJM behaviour (a RIFF header configuring
  an uninitialised ATRAC9 instance, frame lengths, results 0x10/0x8) on
  LibAtrac9. A run (UCR_TRACE_FILTER=Ajm) gets through 436 RunSplitBuffer
  jobs, every BatchWait returning 0, while the logos play.
- Stops at: FATAL "direct memory 12C00000 mapped twice (aliasing): not
  supported" (sceKernelMapDirectMemory from a guest thread, 0x0104),
  after the AJM jobs above. The kernel HLE maps each direct memory range
  once; the game maps one range at two addresses.

## 2026-11-08: direct memory "mapped twice" was munmap; next, flip mode 2

- Not aliasing. The game keeps a linear view of its direct memory (one
  0x64000000 mapping at 0x10_0000_0000 + offset) and moves 1 MiB pieces
  out of it: VirtualQuery, munmap of the piece, MapDirectMemory of the
  same offset at 0x13_0000_0000 + n MiB (FIXED|NO_OVERWRITE, 75 pieces;
  the console's log shows the same sequence: slot 0x1300000000, then
  0x1300100000, ...). For the video decoder it then munmaps the whole
  75 MiB window in one call and maps each offset back into the linear
  view (FIXED|NO_OVERWRITE, from 0x8624BF; 204 calls, no error on the
  console). SwARc's munmap freed only the first mapping of the window
  and kept unmapped pieces reserved; fixed in SwARc (munmap frees every
  mapping in its range, fixed maps reserve their exact address).
- Stops at: FATAL "sceVideoOutSubmitFlip: flip mode 2 not implemented
  yet".

## 2026-11-08: flip mode 2: where it comes from, what it does

- The game's flip thread (0x8FD6A0 on, the SubmitAndFlip call at
  0x8FDBE1) takes queued flip requests; the producer (0x91DDF2) sets the
  flip mode to 2 when the byte at 0x12765C0+0x30 is set, otherwise 1.
  The byte is set by 0xB50BFF and cleared by 0xB50C9D (a paired
  enter/leave on the counter at 0xFA7E30). On our run that happens at
  flip 701 (flip arg 701): flips 0-700 are mode 1.
- The console's run 6 logs only the first 64 SubmitAndFlip calls in full
  (all mode 1, flip rate 0 from SetFlipRate); the summary shows 7949
  calls with no error, so mode 2 is accepted there.
- libSceVideoOut 11.00's SubmitFlip only checks the mode is 1-6
  (0x80290006 otherwise) and passes it to the kernel's flip ioctl
  0xC0487704; what mode 2 does (when the flip happens against the
  vblanks) is decided in the kernel; measured below.
- The game waits on the flip on the GPU (sceGnmInsertWaitFlipDone, the
  buffer's label), so the timing of the flip, and of the label clear, is
  what mode 2 changes for it.
- Measured (console run 7, PS4-HB/dumps/11.00/hlevals_ucr_run7): the
  hlevals plugin samples sceVideoOutGetFlipStatus and
  sceVideoOutGetVblankStatus every 100 us and logs each SubmitAndFlip
  mode change. The game is in mode 2 for flip args 675-800 and
  3899-4639, mode 1 otherwise. Mode 1: every flip at a vblank (flip
  TSC = vblank TSC), one per 16.7 ms. Mode 2: no flip at a vblank; flips
  20-140 us (median 46) after the submit TSC, 6.7-14 ms apart, 1-3 per
  vblank (109 vblanks with 1, 274 with 2, 68 with 3).
- Implemented in SwARc (5918978, "orbis: flip mode 2"). A run passes the
  switch (first mode 2 flip at arg 700) and keeps going past flip 9400
  in 180 s, without a fatal error.
- A run with UCR_TRACE=1 (slower) stopped earlier, at flip 247, with
  FATAL "libSceVideodec2: a second SPS not implemented yet"; the plain
  run did not reach it in 180 s. Next stop to look at.
