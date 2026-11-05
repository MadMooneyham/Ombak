# Building Ombak

Ombak builds as Penumbra does (its platform is Penumbra's). The build makes three files in `build/`:

| File | What |
| --- | --- |
| `ombak.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `ombak.fwsc` | the installable package (app + loader) |

## Prerequisites

- Python 3 with Pillow: `pip3 install Pillow`
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

On Linux x86-64 the toolchain runs natively. On macOS the build runs each tool in a `linux/amd64`
`debian:bookworm-slim` container: Docker (or podman behind a `docker` script) must be running.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations (`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 1.0` makes a release build: the package is `build/ombak-1.0.fwsc`, identity
`FM-1_74010000` (74 is Ombak's; Penumbra's is 73), its version `1.0` (a 0.x would say BETA).

**A release:** build it, then publish a GitHub release tagged `v1.0` with `build/ombak-1.0.fwsc` attached
and `RELEASE-1.0.md` as its notes. Publishing runs `.github/workflows/pages.yml`, which builds the site
(`tools/make_pages.py`: the landing page, the web installer, the reader's words) from that release's own package
and deploys it to GitHub Pages (Settings > Pages > Source: GitHub Actions, once). Locally:
`python3 tools/make_pages.py build/ombak-1.0.fwsc 1.0 build/pages`, then `python3 -m http.server` there.

The build also generates `build/gen/`: the font, the logo (`tools/gen_logo.py`, from `assets/logo.jpg`), the
Ongkara (`tools/gen_ongkara.py`, from `assets/ongkara.png`), the effects' tables and the screen reader's words link.
It fails if a soft-double routine is linked (a `double` crept in) and checks the image, RAM and pool sizes.

## Tests

```
tests/run_scenarios.sh
```

Runs the whole app in the simulator (`tests/scenarios/*.pen`), with its screenshots and audio in
`build/scenarios/`. `tests/click_check.py` and `tests/wav_levels.py` check a recording for clicks and levels.
`host/build_host.sh` builds the simulator alone; `build/host/ombak_host SCRIPT OUTDIR` runs one script (the
command list is at the top of `host/ombak_host.c`). The simulator's timing is the computer's: how hard the FM-1
works is measured on the FM-1 itself, on its BENCH page (GLO, then GLO again; SEL there runs the load test).

## Install

From the command line (needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/ombak-1.0.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

The web installer (https://madmooneyham.github.io/Ombak/install/, or the local site above) takes it in Chrome. Installing firmware is at your own risk. If the FM-1 no longer
starts but reaches the chip's update mode (4C4A:8057 on USB), `tools/fm1_rescue.sh` puts stock V15 back from a
Mac; otherwise recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter). Holding OCT- and
OCT+ for five seconds puts Ombak in update mode, as in Penumbra.
