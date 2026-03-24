Build .tns (Ndless)
===================

Quick steps
-----------

1. Open a terminal in this folder:

   /Users/ethan/code/craftiog/crafti/new/crafti

2. Build:

   make clean && make -j4

3. Output file:

   crafti.tns

Requirements
------------

- Ndless toolchain installed and on PATH:
  - nspire-gcc
  - nspire-g++
  - nspire-ld
  - genzehn
  - make-prg

- Optional: NDLESS_INCLUDES path via NDLSSDK
  - Example:
    export NDLSSDK=/Users/you/Ndless/ndless-sdk

Notes
-----

- The Makefile normalizes the final artifact to `crafti.tns` even if `make-prg`
  auto-generates a versioned filename.
- If build tools are missing, check your Ndless SDK/toolchain installation and
  PATH first.
