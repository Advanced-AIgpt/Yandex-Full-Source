# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from fst_base import NluFstBaseValue


FRACTION_SYM = '/'


class NluFstFloat(NluFstBaseValue):
    @staticmethod
    def _convert_int_frac_part(int_part, frac_part):
        try:
            int, frac = float(int_part), float(frac_part)
            frac = frac / 10 ** len(frac_part)
            value = int + frac
        except ValueError:
            return None
        return value

    def _try_parse_number(self, value):
        value = super(NluFstFloat, self)._try_parse_number(value)
        if isinstance(value, basestring):
            value_items = value.strip().split()
            if FRACTION_SYM in value:
                nom, denom = value.split(FRACTION_SYM, 1)
                try:
                    value = float(nom) / float(denom)
                except ValueError:
                    pass
                except ZeroDivisionError:
                    pass
            elif len(value_items) == 2:
                # interprets like integer and fractional part given separately
                value = self._convert_int_frac_part(*value_items) or value
            elif len(value_items) == 3 and value_items[1] == 'и':
                value = self._convert_int_frac_part(value_items[0], value_items[2]) or value
        return value
