from __future__ import absolute_import

import os
import codecs
import json
import re

from collections import defaultdict, OrderedDict


class IntentRenamer(object):
    """Tool to rename intents. Useful while working with toloka data
    The initial argument, `rename_intents`, is a single filename (or json string) or a list of them.
    If it is a list, then priority of renamings decreases as we move along the list.
    """

    class By(object):
        TRUE_INTENT = 0
        PRED_INTENT = 1

    def __init__(self, rename_intents):
        if isinstance(rename_intents, basestring):
            rename_intents = [rename_intents]
        if rename_intents is None:
            rename_intents = []
        self._rename_intents = [self._get_rename_intents_map(filename) for filename in rename_intents]
        self._rename_intents_cache = [defaultdict(dict) for filename in rename_intents]

    def _get_rename_intents_map(self, rename_intents):
        if not rename_intents:
            return {
                IntentRenamer.By.TRUE_INTENT: {},
                IntentRenamer.By.PRED_INTENT: {}
            }
        if hasattr(rename_intents, 'read'):
            renames = json.load(rename_intents, object_pairs_hook=OrderedDict)
        elif os.path.exists(rename_intents):
            with codecs.open(rename_intents) as f:
                renames = json.load(f, object_pairs_hook=OrderedDict)
        else:
            renames = json.loads(rename_intents, object_pairs_hook=OrderedDict)

        assert isinstance(renames, dict)

        if set(renames.keys()) == {'true_intent', 'pred_intent'}:
            return {
                IntentRenamer.By.TRUE_INTENT: renames['true_intent'],
                IntentRenamer.By.PRED_INTENT: renames['pred_intent']
            }
        else:
            return {
                IntentRenamer.By.TRUE_INTENT: renames,
                IntentRenamer.By.PRED_INTENT: renames
            }

    def __call__(self, intent_name, by):
        if len(self._rename_intents) == 0:
            return intent_name
        if by not in self._rename_intents[0]:
            raise ValueError('Your rename key "{}" is not in rename config'.format(by))

        if intent_name is None:
            return None

        # we return result of the first mapping with successful match
        for expressions, caches in zip(self._rename_intents, self._rename_intents_cache):
            cache = caches[by]

            if intent_name in cache:
                cached_intent_name = cache[intent_name]
                if cached_intent_name is None:
                    # None means that intent does not exist in the current mapping
                    continue
                else:
                    return cached_intent_name
            for regexp, new_intent_name in expressions[by].iteritems():
                if re.match(regexp, intent_name):
                    cache[intent_name] = new_intent_name
                    return new_intent_name
            # we add None as a flag that intent name is not matched by the current mapping
            cache[intent_name] = None
        return intent_name


def get_intent_for_metrics(intent):
    if isinstance(intent, basestring) and 'handcrafted' in intent:
        return 'handcrafted'
    else:
        return intent
