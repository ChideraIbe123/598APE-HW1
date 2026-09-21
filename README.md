# 598APE-HW1 — Optimized Raytracer (Artifact)

This is my optimized version of the course raytracer for Mini-Paper 1. This README explains how to build it, run it, benchmark it, and check each optimization on its own. 

No special hardware is needed: everything runs on a standard Unix machine or through the provided Docker setup. OpenMP is the only parallelism used (plain `-fopenmp`, no intrinsics), so the speedups reproduce anywhere — absolute times will differ by machine, but the relative improvements should look the same.

## 1. Setup

Everything runs inside Docker so you don't need to install anything except Docker itself.

```bash
docker build -t 598ape docker/     # build the image (gcc, valgrind, ImageMagick, ffmpeg, etc.)
bash dockerrun.sh                  # start a container; the repo is mounted at /host
cd /host                           # inside the container
```

Note: `dockerrun.sh` refers to the image by my Docker Hub name. If you built the image yourself with the command above, either edit the image name in `dockerrun.sh` to `598ape`, or run:

```bash
docker run -it --security-opt seccomp=unconfined -v "$(pwd):/host" 598ape /bin/bash
```

## 2. Build and run

```bash
make -j
```

Heads up: if you ever change compiler flags in a Makefile, run `make clean && make -j` — make does not notice Makefile edits on its own.

Run the three scenes (these are also the benchmark commands used in the paper):

```bash
# pianoroom, single 500x500 frame
./main.exe -i inputs/pianoroom.ray --ppm -o output/pianoroom.ppm -H 500 -W 500

# globe, single 500x500 frame
./main.exe -i inputs/globe.ray -a inputs/globe.animate --ppm --no-movie -F 1 -H 500 -W 500 -o output/globe.ppm

# elephant/mesh, 24-frame animation at 100x100
./main.exe -i inputs/elephant.ray --ppm -a inputs/elephant.animate --movie -F 24 -W 100 -H 100 -o output/elephant.mp4
```

Each run prints `Total time to create images=...` — that's the number the paper reports. Times vary a bit run to run, so the paper uses the best of 3.

## 3. Verifying correctness

Every optimization was checked by byte-comparing rendered output against the unoptimized code. To reproduce that check yourself:

```bash
# inside the container, git needs this once before touching the mounted repo:
git config --global --add safe.directory '"'"'*'"'"'

# build the baseline in a worktree inside the repo (works in the container too)
git worktree add baseline-check 3bbf137
cd baseline-check && make -j
./main.exe -i inputs/pianoroom.ray --ppm -o baseline.ppm -H 100 -W 100
cd ..

# render the same frame with the optimized build and compare
./main.exe -i inputs/pianoroom.ray --ppm -o output/test.ppm -H 100 -W 100
cmp output/test.ppm baseline-check/baseline.ppm && echo IDENTICAL

# clean up when done
git worktree remove baseline-check --force
```

The same works for the other scenes (for animations, compare the per-frame `.ppm` files). Because the final build is multithreaded, it's also worth rendering twice and comparing the two runs to each other — identical output shows there are no data races.

## 4. The optimizations, one commit each

Every optimization is a single commit, so you can measure any intermediate state by checking it out (the worktree trick above works for any hash). The table shows instruction counts from `valgrind --tool=callgrind` (the deterministic "I refs" line) for pianoroom at 500x500.

| # | commit | what it does | kind | pianoroom Ir |
|---|--------|--------------|------|--------------|
| — | `3bbf137` | baseline (formatting only) | — | 14.30 B |
| 1 | `b24c234` | replace insertion sort with a min scan in `calcColor` | algorithmic | 14.09 B |
| 2 | `e4baf84` | cache the ray-invariant `solveScalers` coefficients per shape | caching | 12.01 B |
| 3 | `42e0c2e` | `-O0` → `-O3` in `src/Makefile` | compiler | 5.30 B |
| 4 | `ea914bf` | remove the per-ray malloc/copy/free of the intersection array (also fixes a leak) | work removal | 3.16 B |
| 5 | `7a8ffc8` | `-flto` in all three Makefiles (cross-file inlining) | compiler | 1.43 B |
| 6 | `c1cfb23` | OpenMP `schedule(dynamic, 64)` on the pixel loop | parallelism | ~1.43 B, time −89% |
| 7 | `2817a92` | axis-aligned bounding box around each mesh; rays that miss it skip all triangles | data structure | (mesh scenes only) |

Notes on reading the table:
- Opt 6 doesn't reduce instructions — the same work runs on more cores — so its effect shows in wall-clock time, not Ir.
- Opt 7 doesn't affect pianoroom (no meshes). On the elephant scene it cuts `Triangle::getIntersection` calls from 9.1M to 0.3M per 50x50 frame and the 24-frame benchmark from 1.587s to 0.036s.

To profile any build yourself:

```bash
valgrind --tool=callgrind ./main.exe -i inputs/pianoroom.ray --ppm -o output/p.ppm -H 500 -W 500
# the "I refs:" line at the end is the instruction count;
# open the callgrind.out.* file in kcachegrind/qcachegrind to see per-function costs
```

(Times printed under valgrind are ~30x inflated by instrumentation — use them for attribution only, never as benchmark numbers.)

## 5. Overall results (from the paper)

Measured on a 14-core Apple Silicon machine inside Docker, best of 3:

| scene | baseline | optimized | speedup |
|-------|----------|-----------|---------|
| pianoroom (500x500) | 1.224 s | 0.023 s | 53x |
| globe (500x500, 1 frame) | 0.709 s | 0.019 s | 37x |
| elephant (100x100, 24 frames) | 886 s (first 8 frames only — see caveat below) | 0.036 s | >24,000x |

One caveat worth knowing if you benchmark the baseline yourself: the unoptimized code leaks memory on every sky ray (fixed in Opt 4), so long baseline runs — like the full 24-frame elephant animation — can exhaust container memory and get killed before finishing. Shorter runs complete fine.
