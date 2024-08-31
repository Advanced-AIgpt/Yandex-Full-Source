# coding: utf-8
from __future__ import unicode_literals

import copy
import logging

from collections import defaultdict

from alice.nlu.py_libs.utils.slot import group_by_slot
from vins_core.common.entity import Entity

logger = logging.getLogger(__name__)


def get_slot_value(slot, of_type):
    entities = filter(lambda e: e.type == of_type, slot['entities'])
    if not entities:
        return None
    else:
        return entities[0].value


def _match_slot_with_entity(frame_slot, entity, target_type, matching_type):
    """Helper for matching types and positions between slot and entity.

    -- frame_slot has format {'substr': 'blabla', 'start': 0, 'end': 1, 'entities': list of entities}
    -- entity has format {'start': 0, 'end': 1, 'value': 'Москва', 'type': 'GEO'}
    """
    slot_start, slot_end = frame_slot['start'], frame_slot['end']
    ent_start, ent_end = entity.start, entity.end

    type_match = entity.type.lower() == target_type
    if matching_type == 'exact':
        position_match = (ent_start == slot_start and ent_end == slot_end)
    elif matching_type == 'inside':
        position_match = (ent_start >= slot_start and ent_end <= slot_end)
    elif matching_type == 'overlap':
        position_match = (ent_end > slot_start and ent_start < slot_end)
    else:
        raise ValueError("matching_type value can be 'exact', 'inside' or 'overlap'. You passed %s" % matching_type)

    is_matched = position_match and type_match
    overlap_size = max(0, min(slot_end, ent_end) - max(slot_start, ent_start))

    return is_matched, overlap_size


def matched(frame_slot, target_types, matching_type, return_subentity=True):
    """
    Iterate through entities in semantic frame slot that match specified target types.
    :param frame_slot: semantic frame slot
    :param target_types: list of target types
    :param matching_type: type of matching slot with entities.
        -- 'exact' (default) means that entity must have the same 'start', 'end' positions as slot has.
        -- 'inside' means that entity must be contained in slot.
        -- 'overlap' means that entity can overlap with slot.
    :param return_subentity: if True, sub-entity is returned if type is hierarchical (e.g. type.subtype)
    :return:
    """
    if not target_types:
        # nothing to be matched
        return

    for target_type in target_types:
        if target_type == 'datetime_raw':
            target_type = 'datetime'
        if target_type == 'datetime_range_raw':
            target_type = 'datetime_range'
        if target_type == 'string':
            # string value is always present
            yield Entity(
                start=frame_slot['start'],
                end=frame_slot['end'],
                type=target_type,
                value=frame_slot['substr']
            )
        elif '.' in target_type:
            # hierarchical entity, only 2-level
            ttype, tsubtype = target_type.split('.', 1)
            for entity in frame_slot['entities']:
                if entity.type.lower() == ttype and tsubtype in entity.value:
                    if return_subentity:
                        sub_entity = copy.deepcopy(entity)
                        sub_entity.value = sub_entity.value[tsubtype]
                        yield sub_entity
                    else:
                        yield entity
        else:
            # ordinary entity
            found_entities = []
            for entity in frame_slot['entities']:
                is_matched, overlap_size = _match_slot_with_entity(frame_slot, entity, target_type, matching_type)
                if is_matched:
                    found_entities.append((entity, overlap_size))
            # prefer entities with larger overlap
            found_entities.sort(key=lambda x: x[1], reverse=True)
            for entity, _ in found_entities:
                yield entity


def tags_to_slots(tokens, tags, entities=None):
    """
    Converts lists of tokens and tags into dict object with slots and their values (optional)
    :param tokens: list of tokens
    :param tags: list of tags to each token
    :param entities: dict of entities, each entity contains "start", "end" and "value" fields
    :return: (slots, free_entities) tuple where:
        - 'slots' (dict): keys are extracted tags and values are lists of dict objects (one for each slot occurrence)
        - 'free_entities' (list of dicts): entities that are not belong to any tag
    """
    entities = entities or []
    slots, free_entities = defaultdict(list), []

    for slot_tokens, slot_idx, slot_tags, slot_key in group_by_slot(tokens, tags):
        start, end = slot_idx[0], slot_idx[-1] + 1
        if slot_key == 'O':
            free_entities.extend(filter(lambda e: e.start >= start and e.end <= end, entities))
            continue
        new_slot = dict(
            substr=' '.join(slot_tokens),
            start=start,
            end=end,
            # Add entities which overlap with slot. Following behaviour depends on "matching_type" of slot.
            # See SlotTypeFrameScoring class.
            # deepcopy ensures each frame holds its own entities.
            entities=copy.deepcopy(filter(lambda e: e.end > start and e.start < end, entities)),
            is_continuation=slot_tags[0].startswith('I-')
        )
        slots[slot_key].append(new_slot)

    return slots, free_entities


def _inner_entities(slot):
    return [entity for entity in slot['entities'] if entity.start >= slot.start and entity.end <= slot.end]


def merge_slots(prev_slot, next_slot, entity_matching_type='exact'):
    """
    Make a single slot dict out of two slot dicts, with correct selection of entities within the joint slot.
    Matching types are the same as in vins_core.common.slots_map_utils._match_slot_with_entity function.
    """
    if entity_matching_type == 'exact':
        new_entities = []
    elif entity_matching_type == 'inside':
        new_entities = _inner_entities(prev_slot) + _inner_entities(next_slot)
    elif entity_matching_type == 'overlap':
        # some entities might be duplicated, but it is ok, because only one is finally chosen
        new_entities = prev_slot['entities'] + next_slot['entities']
    else:
        logger.warning('Matching type "{}" was not recognized, "exact" assumed'.format(entity_matching_type))
        new_entities = []
    new_slot = {
        'start': prev_slot['start'],
        'end': next_slot['end'],
        'entities': new_entities,
        'substr': prev_slot['substr'] + ' ' + next_slot['substr'],
        'is_continuation': prev_slot['is_continuation']
    }
    return new_slot
