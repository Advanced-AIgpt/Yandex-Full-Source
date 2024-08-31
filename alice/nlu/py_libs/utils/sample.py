# coding: utf-8

from alice.nlu.py_libs.utils import strings
from alice.nlu.py_libs.utils.slot import group_by_slot

import attr
from operator import attrgetter


@attr.s
class Sample(object):
    tokens = attr.ib()
    tags = attr.ib()

    def __init__(self, tokens, tags=None):
        self.tokens = tokens
        self.tags = tags or ['O'] * len(tokens)

    @property
    def text(self):
        return ' '.join(self.tokens).strip()

    @classmethod
    def from_string(cls, s, tokens, tags):
        text = strings.smart_unicode(s)
        tokens = tokens or text.split()
        tags = tags or ['O'] * len(tokens)
        return cls(tokens=tokens, tags=tags)

    @classmethod
    def from_nlu_source_item(cls, item):
        tokens, tags = [], []

        text = item.text
        if not text:
            return cls(tokens=tokens, tags=tags)

        slots = list(sorted(item.slots, key=attrgetter('begin')))

        cur_pos = 0
        end_pos = len(text)
        while cur_pos < end_pos:
            if slots:
                slot = slots.pop(0)
                if slot.begin > cur_pos:
                    for token in text[cur_pos:slot.begin].strip().split():
                        tokens.append(token)
                        tags.append('O')
                    cur_pos = slot.begin
                prefix = 'I-' if slot.is_continuation else'B-'
                for token in text[slot.begin:slot.end].strip().split():
                    tokens.append(token)
                    tags.append('{0}{1}'.format(prefix, slot.name))
                    prefix = 'I-'
                cur_pos = slot.end
            else:
                for token in text[cur_pos:end_pos].strip().split():
                    tokens.append(token)
                    tags.append('O')
                cur_pos = end_pos

        return cls.from_string(item.text, tokens=tokens, tags=tags)

    def __len__(self):
        return len(self.tokens)

    def group_by_slot(self):
        # may be one day external group_by_slot won't be used wo Sample (currently in vins)
        return group_by_slot(self.tokens, self.tags)

    def to_markup(self):
        text_pieces = []
        for slot_toks, _, slot_tags, group_tag in self.group_by_slot():
            if group_tag == 'O':
                text_pieces.append(' '.join(slot_toks))
                continue
            slot_name = (u'+' if slot_tags[0].startswith('I-') else '') + group_tag
            text_pieces.append(u"'{}'({})".format(' '.join(slot_toks), slot_name))
        return u' '.join(text_pieces)

    def with_tags(self, tags):
        return Sample(self.tokens, tags)
