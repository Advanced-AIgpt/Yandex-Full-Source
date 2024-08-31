# coding: utf-8
from __future__ import unicode_literals

import re

from copy import deepcopy
from collections import defaultdict

from vins_core.ner.fst_datetime_ru import NluFstDatetimeRu


class NluFstDatetimeRange(NluFstDatetimeRu):

    _TAPE_PATTERN = r'(?:s|e):(?:\d+\s+\w[\+\-]?)+'

    def to_datetime(self, values):
        output = {}
        for value in values:
            out = defaultdict(int)
            rel_out = []
            spec_out = {}
            for v, f, pm in re.findall(r'(\d+)\s+(\w)([\-\+]?)', value):
                v = int(v)
                if f == 'E':
                    spec_out['weekend'] = True
                    # if weekend in relative, then time is measured in weeks
                    f = 'w'
                if f == 'O':
                    spec_out['holidays'] = True
                    # holidays cannot be relative
                else:
                    if pm:
                        rel_out.append((f, v if pm == '+' else -v))
                    else:
                        out[f] = v

            out = self.apply_rel_out(out, rel_out)
            out.update(spec_out)
            if value.startswith('s'):
                output['start'] = out
            elif value.startswith('e'):
                output['end'] = out

        if 'start' not in output:
            output['start'] = deepcopy(output['end'])
            for key in output['start']:
                if isinstance(output['start'][key], int) and '_relative' not in key:
                    output['start'][key] = 0

        return output
