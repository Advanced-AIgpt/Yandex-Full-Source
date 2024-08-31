# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.test import TestCase

from models import VoiceLogSelector


class VoiceLogSelectorTest(TestCase):

    def test_query(self):
        vs = VoiceLogSelector(st='2018-05-28 00:00', et='2018-05-28 23:59')
        self.assertEqual(vs.get_conditions(), "fielddate = '2018-05-28'")

        vs = VoiceLogSelector(st='2018-05-28 00:00', et='2018-05-29 00:00')
        self.assertEqual(vs.get_conditions(), "fielddate = '2018-05-28'")

        vs = VoiceLogSelector(st='2018-05-28 00:00', et='2018-05-29 00:01')
        self.assertEqual(
            vs.get_conditions(),
            "fielddate >= '2018-05-28' AND fielddate <= '2018-05-29' AND ts <= 1527552119"
        )

        vs = VoiceLogSelector(st='2018-05-28 00:00', et='2018-05-28 23:59', reply='такой')
        self.assertEqual(vs.get_conditions(), "positionCaseInsensitiveUTF8(reply, 'такой') > 0 AND fielddate = '2018-05-28'")

        vs = VoiceLogSelector(st='2018-05-28 00:00', et='2018-05-28 23:59', reply='так.*же')
        self.assertEqual(vs.get_conditions(), "match(lowerUTF8(reply), 'так.*же') AND fielddate = '2018-05-28'")

