# coding: utf-8

import argparse
import sys


def find_diff(baseline_lines, updated_lines):
    diff_lines = []
    for baseline_line, updated_line in zip(baseline_lines, updated_lines):
        if baseline_line != updated_line:
            diff_lines.append((baseline_line, updated_line))

    return diff_lines


def check_diff(baseline_lines, updated_lines, intent):
    diff_lines = find_diff(baseline_lines, updated_lines)
    if len(diff_lines) > 0.1 * len(baseline_lines):
        print(intent)
        for baseline_line, updated_line in diff_lines:
            print('-' + baseline_line)
            print('+' + updated_line)
        return True
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('baseline', type=str)
    parser.add_argument('tested', type=str)

    args = parser.parse_args()

    with open(args.baseline) as f:
        baseline_lines = list(map(lambda line: line.strip(), f.readlines()))

    with open(args.tested) as f:
        updated_lines = list(map(lambda line: line.strip(), f.readlines()))

    assert len(updated_lines) == len(baseline_lines)

    intent, prev_pos = None, None
    has_diff = False
    for pos, line in enumerate(baseline_lines):
        if line.startswith('Intent: '):
            if prev_pos:
                has_diff = has_diff or check_diff(baseline_lines[prev_pos: pos], updated_lines[prev_pos: pos], intent)

            intent = line.split()[1]
            prev_pos = pos

    has_diff = has_diff or check_diff(baseline_lines[prev_pos:], updated_lines[prev_pos:], intent)

    sys.exit(has_diff)


if __name__ == "__main__":
    main()
