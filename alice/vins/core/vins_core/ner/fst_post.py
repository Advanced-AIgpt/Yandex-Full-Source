# coding: utf-8

from vins_core.ner.fst_base import HIERARCHY, Entity


class NluFstPost(object):

    def __init__(self, type_prefix=''):
        self.type_prefix = type_prefix

    def __call__(self, entity_lists, utt, *args, **kwargs):
        entities = (self.combine_entities(self.ner_reformat(entity_list))
                    for entity_list in entity_lists.itervalues())

        ners = sum(entities, [])
        entities_start = [ner.start for ner in ners]

        return [{
            'delex': [],
            'ners': zip(entities_start, ners),
            'utt': utt
        }]

    @staticmethod
    def ner_reformat(entity_list, substr=False):
        ners = []
        for entity in entity_list:
            if not entity.type:
                continue
            if HIERARCHY in entity.type:
                etype, eprop = entity.type.split(HIERARCHY)  # TODO: multi-level hierarchy
            else:
                etype, eprop = entity.type, ''
            e = Entity(
                start=entity.start,
                end=entity.end,
                type=etype,
                value=entity.value if not eprop else {eprop.lower(): entity.value},
                weight=entity.weight,
                substr=entity.substr if substr else ''
            )
            ners.append(e)

        return ners

    @staticmethod
    def combine_entities(entity_list):
        toks = []
        for e in sorted(entity_list, key=lambda item: item.start):

            if len(toks) == 0:
                toks.append(e)
                continue

            if not isinstance(e.value, dict):
                # integral types are not merged
                toks.append(e)
                continue

            eprop = e.value.keys()[0]
            prev_tok = toks[-1]
            if prev_tok.end == e.start and eprop not in prev_tok.value:
                prev_tok.end = e.end
                prev_tok.value.update(e.value)
            else:
                toks.append(e)
        return toks
