#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('.')

from general.normbase import *
import categories
import importlib
import os
import shutil
import re
import argparse

usage_help_message = "Usage: path/to/file/with/sequance/of/FSTs \noptional: --reverse"

parser = argparse.ArgumentParser(
    description=(
        "Build normalizer or reverse normalizer.\n"
        "Reads the config file with list of requested transforms and generate corresponding FSTs chain.\n"
    )
)
parser.add_argument('config_file', help='path to file with config')
parser.add_argument(
    '--reverse', dest='reverse', default=False, action='store_true',
    help='build reverse normalizer FSTs chain'
)
options = parser.parse_args()

if not os.path.exists(options.config_file):
    sys.exit("ERROR: File %s was not found!" % options.config_file)

current_path = os.path.dirname(os.path.abspath(__file__))

if options.reverse:
    target_folder = "revnorm"
    sys.path.append(os.path.join(current_path, 'reverse-normalization'))
else:
    target_folder = "norm"
    sys.path.append(os.path.join(current_path, 'normalization'))

shutil.rmtree(target_folder, ignore_errors=True)
os.mkdir(target_folder)

fst_syms_save(os.path.join(target_folder, "symbols"))

with open(options.config_file) as input_file:
    with open(os.path.join(target_folder, "sequence.txt"), "w") as of:
        def save(in_fst, fst_name):
            fst_save(in_fst, os.path.join(target_folder, fst_name))
            print >>of, fst_name

        for line in input_file:
            line = line.strip()
            if line[0] == '#':	# comment
                continue
            line_parts = line.split('.')
            module_name = line_parts[0]
            if len(line_parts) < 2:
                sys.exit("ERROR: Fst in module %s was not specified correctly!" % line)
            fst_name = line.strip()[len(module_name) + 1:]
            fst_name = re.sub("\(.*\)", "", fst_name)
            module = importlib.import_module(module_name)
            save(eval("module" + line.strip()[len(module_name):]), module_name + "." + fst_name)

with open(os.path.join(target_folder, "flags.txt"), "w") as ff:
    print >>ff, "report-intermediate false"
