#!/usr/bin/python
# coding=utf-8
import re
from alice.boltalka.tools.reply_rewriter.base_replacer import BaseReplacer

class Capitalizer(BaseReplacer):
    def __init__(self, args):
        self.eos_symbols = '.?!'

    def process(self, reply):
        new_reply = ''
        was_eos = True
        for c in reply:
            if c in self.eos_symbols:
                was_eos = True
            elif not c.isspace() and was_eos:
                c = c.upper()
                was_eos = False
            new_reply += c
        return new_reply
