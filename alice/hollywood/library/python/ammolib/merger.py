import os


def merge_text_files_into_one(src_filenames, dst_filename):
    with open(dst_filename, "w") as dst_f:
        for src_filename in src_filenames:
            with open(src_filename, "r") as src_f:
                for line in src_f:
                    line = line.strip()
                    if line == "":
                        continue
                    dst_f.write(line + "\n")


def split_text_file_into_multiple_by_lines(src_filename, dst_dirname):
    result_filenames = []
    src_basename = os.path.basename(src_filename)
    with open(src_filename, "r") as src_f:
        for index, line in enumerate(src_f):
            line = line.strip()
            if line == "":
                continue
            dst_filename = os.path.join(dst_dirname, "{}_{}".format(src_basename, index))
            with open(dst_filename, "w") as dst_f:
                dst_f.write(line + "\n")
            result_filenames.append(dst_filename)
    return result_filenames
