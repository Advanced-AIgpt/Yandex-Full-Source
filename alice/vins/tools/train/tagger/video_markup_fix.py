# coding: utf-8

"""
Fixes few common mistakes in toloka markup for video tagger
"""

import click
import re
from codecs import open
from functools import partial


def replace(match, replacement_format):
    return replacement_format.format(*match.groups())


def fixline(line, patterns):
    for pattern, replacement_format in patterns:
        replace_func = partial(replace, replacement_format=replacement_format)

        line = pattern.sub(replace_func, line)

    return line


@click.command()
@click.option('--input-path', type=click.Path(exists=True), help='Path source nlu file')
def do_main(input_path):
    patterns = [
        (re.compile(ur"(нов[а-я]{2})'\(new\) (сери[а-я])( |$)"), u"{}'(episode) {}{}"),
        (re.compile(ur"(нов[а-я]{2,3})'\(new\) (сезон[а-я]{0,2})"), u"{}'(season) {}"),
        (re.compile(ur"'\(release_date\) (год[а-я]{0,2})"), u" {}'(release_date)"),
    ]

    lines = []
    with open(input_path, encoding='utf8') as f:
        for line in f:
            lines.append(fixline(line.strip(), patterns))

    with open(input_path, 'w', encoding='utf8') as f:
        for line in lines:
            f.write(line + '\n')


if __name__ == '__main__':
    do_main()
