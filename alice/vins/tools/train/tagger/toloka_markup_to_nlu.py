#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from toloka_markup import TolokaMarkup


def make_nlu_line(toloka_markup):
    text = toloka_markup.text
    begin = 0
    result_parts = []
    for tag in toloka_markup.tags:
        result_parts.append(text[begin:tag.span.pos])
        tag_span_end = tag.span.pos + tag.span.length
        nlu_slot_value = u"'{0}'({1})".format(text[tag.span.pos:tag_span_end], tag.label)
        result_parts.append(nlu_slot_value)
        begin = tag_span_end
    result_parts.append(text[begin:])
    return ''.join(result_parts)


def no_markup(line):
    return line.rstrip().split('\t')[1] == 'NOTHING_TO_TAG'


def do_main():
    for line in sys.stdin:
        if not line.strip() or no_markup(line):
            continue
        print make_nlu_line(TolokaMarkup.deserialize(line.rstrip().decode('utf-8'))).encode('utf-8')

    return 0


if __name__ == '__main__':
    sys.exit(do_main())
