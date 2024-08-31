# coding: utf-8

import attr
import re


@attr.s(slots=True, frozen=True, cmp=True)
class Slot(object):

    name = attr.ib()
    begin = attr.ib()
    end = attr.ib()
    is_continuation = attr.ib(default=False, converter=bool)
    options = attr.ib(default=(), converter=tuple)


def group_by_slot(tokens, tags):
    """ Split sentence into same-slot sequences BI+ (normal slots) or I+ (continued slots) and iterate over them """
    slot_toks, slot_idx, slot_tags = [], [], []
    prev_tag = None
    for idx, (token, raw_tag) in enumerate(zip(tokens, tags)):
        tag = re.sub('^(B-|I-)', '', raw_tag)
        if tag != prev_tag or raw_tag.startswith('B-'):
            if prev_tag is not None:
                yield slot_toks, slot_idx, slot_tags, prev_tag
                slot_toks, slot_idx, slot_tags = [], [], []
        slot_toks.append(token)
        slot_idx.append(idx)
        slot_tags.append(raw_tag)
        prev_tag = tag
    if prev_tag is not None:
        yield slot_toks, slot_idx, slot_tags, prev_tag
