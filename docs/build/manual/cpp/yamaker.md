# yamaker

## Overview

Yamaker is an almost zero configuration C/C++ contrib importer.

It observes an original build process using [fptrace](https://github.com/orivej/fptrace) and recreates it with `ya.make` files. It runs only on Linux, but provides facilities for altering the result to support any platform.

It takes package definitions from [Nixpkgs](https://nixos.org/nixpkgs/), so in most cases you can name a package and yamaker will know how to build it.

There is [yamaker public support group](https://t.me/joinchat/DNx58UnZG7lWFQa_B-FVVA) on Telegram.

[Анонс](https://wiki.yandex-team.ru/users/rnefyodov/yamaker/)
[Пример](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/yamaker/doc/bento/README.md)
[Tips&Tecks](https://wiki.yandex-team.ru/users/kikht/yamaker-tips-and-tricks/)

## Setup

Please install [Nix](https://nixos.org/nix/) from [`./nix-install.sh`](./nix-install.sh) which launches the official installer after improving the defaults for Yandex; see the end of this script for specifics. The installer has to run under a user with `sudo`, but later `sudo` is never used. If you are using `zsh`, add `source /etc/profile.d/nix.sh` to your `~/.zshrc`.

See the [Nix manual](https://nixos.org/nix/manual/#sect-multi-user-installation) or the output of the installer for the explanation of what it does.

## Usage

🕮 **See [Adding Bento4: an illustrated guide](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/yamaker/doc/bento/README.md), [Обновление musl с помощью yamaker](https://clubs.at.yandex-team.ru/arcadia/20300), [Nix and Nixpkgs how-to](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/yamaker/doc/nix.md), [Project files](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/yamaker/doc/files.md
)**

1. Run `yamaker list` to list projects defined in `projects.py`.
2. Run e.g. `yamaker import contrib/libs/llvm4` under arcadia source root to create or replace `contrib/libs/llvm4`.
3. While working on `yamaker`, run `yamaker import` with `-nb` to skip a rebuild when you change only the install phase.  Use [`Y_PYTHON_SOURCE_ROOT`](https://wiki.yandex-team.ru/arcadia/python/pysrcs/#ypythonsourceroot) to skip rebuilding `yamaker` itself.

### Subcommands

`yamaker list` lists defined projects.

`yamaker import` is the end user interface that orchestrates:

- Nix to build a project from sources
- `fptrace` to record the original build process
- `yamaker run` to recreate the build process with `ya.make`
- postprocessing, such as copying additional files, applying patches, normalizing line endings

`yamaker grep` processess `ya make` output to help with unresolved includes. See [DEVTOOLS-4782](https://st.yandex-team.ru/DEVTOOLS-4782) for details on how to deal with them.

`yamaker run` is a computation engine that processes [fptrace](https://github.com/orivej/fptrace) output to figure out how to describe build process with `ya.make` files.

## Technical details

🕮 **See [Устройство yamaker](https://orivej.at.yandex-team.ru/123/)**

`yamaker` was designed to investigate and realize the hypothesis that almost any realistic build process can automatically be observed and expressed in `ya.make`.

To achieve this `yamaker` may generate RUN_PROGRAM macros: for example, when clang builds `clang-tblgen` and uses it with `llvm-tblgen` to generate include files for its libraries.

During `yamaker import project`, yamaker locates project definition and runs its `build` and `install` phases:

- build for CMake-based projects
    - fetches `ninja` and `fptrace`
      (ninja makes it simple to list available targets)
    - configures the project with `cmake`
    - builds a subset of project targets under fptrace
      so that fptrace does not observe build of unneeded targets
        - fptrace records commands being run, their input and output file paths
- install
    - fetches `dos2unix`
    - runs `yamaker run` on `fptrace` output
      to generate `ya.make`s and to copy files into the destination directory
    - copies additional files and applies patches in the destination
    - formats destination files with `dos2unix`
