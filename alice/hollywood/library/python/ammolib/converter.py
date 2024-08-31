import errno
import json
import os


def convert(src_filenames, dst_path):
    _mkdirs(dst_path)
    dst_filenames = []
    for src_filename in src_filenames:
        dst_filename = _convert(src_filename, dst_path)
        dst_filenames.append(dst_filename)
    return dst_filenames


def _convert(src_filename, dst_path):
    dst_filename = os.path.join(dst_path, os.path.basename(src_filename))
    with open(dst_filename, "w") as dst_f:
        with open(src_filename, "r") as src_f:
            for line in src_f:
                line = line.strip()
                if line == "":
                    continue
                obj = {"Data": line}
                json_obj = json.dumps(obj)
                dst_f.write(json_obj + "\n")
    return dst_filename


def _mkdirs(path):
    if not os.path.exists(path):
        try:
            os.makedirs(path)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise
