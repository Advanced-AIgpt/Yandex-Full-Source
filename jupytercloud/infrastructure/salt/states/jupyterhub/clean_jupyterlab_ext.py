#!/usr/bin/env python3

import argparse

from pathlib import Path
from distutils.version import LooseVersion

EXTENSIONS_DIR = "share/jupyter/lab/extensions"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--venv-dir", required=True)
    parser.add_argument("--extensions", nargs="+", required=True)
    args = parser.parse_args()

    dir = Path(args.venv_dir) / EXTENSIONS_DIR

    assert dir.exists(), f"extensions directory {dir} does not exists"

    for ext in args.extensions:
        versions = []

        for file in dir.glob(f"{ext}-*.tgz"):
            raw_version = file.stem[len(ext) + 1 :]
            version = LooseVersion(raw_version)

            versions.append((version, file))

        if len(versions) < 2:
            continue

        versions.sort()

        versions_to_delete = versions[:-1]
        files_to_delete = [f for _, f in versions_to_delete]

        for file in files_to_delete:
            file.unlink()


if __name__ == "__main__":
    main()
