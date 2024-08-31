#!/usr/bin/env python
import os


ignore_exts = (".py", ".pyc", ".make", ".a")
resources_dir = os.path.join(os.path.dirname(__file__), "resources")
os.path.splitext


def is_valid(filename):
    for ext in ignore_exts:
        if filename.endswith(ext):
            return False
    return True


def resources():
    print("Resource directory: {}".format(resources_dir))

    valid_extensions = set()
    for dirpath, dirnames, filenames in os.walk(resources_dir):
        for filename in filenames:
            if is_valid(filename):
                src_path = os.path.relpath(os.path.join(dirpath, filename), resources_dir)
                dst_path = os.path.join("/frontend", src_path)
                print("Add file '{}' as '{}'".format(src_path, dst_path))
                yield src_path, dst_path
                valid_extensions.add(os.path.splitext(src_path)[1])
    print("Valid extensions: {}".format(valid_extensions))


with open(os.path.join(resources_dir, "ya.make"), "w") as f:
    f.write("PY3_LIBRARY()\n")
    f.write("OWNER(g:voicetech-infra)\n")
    f.write("\n")
    f.write("RESOURCE(\n")
    f.write("\n".join("    {} {}".format(s, d) for s, d in resources()))
    f.write("\n)\n")
    f.write("\n")
    f.write("END()\n")
