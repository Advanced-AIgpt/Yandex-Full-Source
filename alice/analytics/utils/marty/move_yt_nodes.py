#!/usr/bin/env python
import json
import sys
import subprocess as sp
import yt.wrapper as yt
from os.path import join

path = sys.argv[1]
dst_path = sys.argv[2]
tables = yt.list(path, absolute=True)

from_date = sys.argv[3].strip()
for table in tables:
    date = table.split('/')[-1]
    dst_table = join(dst_path, date)
    if date > from_date:
        print date, table, dst_table
        yt.move(table, dst_table, force=True)

# python move_yt_nodes.py //home/voice/robot-voice-qa/nirvana //home/voice/robot-voice-qa/tmp/nirvana 2019-10-10
