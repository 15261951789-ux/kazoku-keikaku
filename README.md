# 家族計画 ～追憶～ Switch Port

D.O. SM2MPX engine visual novel ported to Nintendo Switch homebrew.

## Stage 1
Libnx verification build: console output, romfs read, pad input, save write.

## Build
GitHub Actions (`.github/workflows/build-switch.yml`) builds automatically.
Manual: `make` inside devkitPro MSYS2 shell with `switch-dev` installed.

## Roadmap
- Stage 1: build env verification (this)
- Stage 2: SDL2 renderer + ISF script VM + GGD/GGA decoders
- Stage 3: full game data (romfs) + NSP packaging
