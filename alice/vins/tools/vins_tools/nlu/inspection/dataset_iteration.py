
import codecs
import json
import pandas as pd

from collections import OrderedDict

import yt.wrapper as yt
from vins_core.utils.config import get_setting
from personal_assistant.intents import is_internal, parent_intent_name

from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.dm.request import create_request
from vins_core.utils.misc import gen_uuid_for_tests

from vins_tools.nlu.inspection.nlu_result_info import UtteranceInfo


_yt_client = yt.YtClient(
    proxy=get_setting('YT_PROXY', default='hahn', prefix=''),
    token=get_setting('YT_TOKEN', default='', prefix='')
)

_dummy_req_info = create_request(gen_uuid_for_tests())


def _personal_assistant_is_internal(intent_name):
    return is_internal(intent_name)


def _personal_assistant_parent_intent_name(intent_name):
    return parent_intent_name(intent_name)


def _iterate_plain_text(input_file):
    with codecs.open(input_file, "r", encoding="utf-8") as f_in:
        for line in f_in:
            line = line.strip()
            if line:
                yield line, UtteranceInfo(), None


def _iterate_tsv(
    input_file, text_col='text', intent_col='intent', prev_intent_col=None,
    weight_col=None, device_state_col=None, additional_columns=None
):
    header = pd.read_csv(input_file, encoding='utf-8', sep='\t', nrows=5)
    assert {text_col, intent_col}.issubset(set(header.columns))
    # the data may be too large for pandas, so we will read it iteratively
    for chunk in pd.read_csv(input_file, encoding='utf-8', sep='\t', chunksize=1000):
        for _, item in chunk.iterrows():
            prev_intent = None
            if prev_intent_col:
                prev_intent = item[prev_intent_col]
            elif _personal_assistant_is_internal(item[intent_col]):
                # TODO: make this compatible with non-PA intents configuration
                prev_intent = _personal_assistant_parent_intent_name(item[intent_col])
            if weight_col:
                weight = float(item[weight_col])
            else:
                weight = 1
            if not isinstance(prev_intent, basestring):
                prev_intent = None
            if device_state_col:
                device_state = item[device_state_col]
                if pd.isnull(device_state):
                    device_state = {}
                else:
                    device_state = json.loads(device_state)
            else:
                device_state = None
            if additional_columns is None:
                additional_data = None
            else:
                additional_data = OrderedDict()
                for c in additional_columns:
                    additional_data[c] = item[c]
            yield item[text_col], UtteranceInfo(
                true_intent=item[intent_col],
                prev_intent=prev_intent,
                device_state=device_state,
                weight=weight,
                additional_data=additional_data
            ), None


def _iterate_nlu(input_file):
    with codecs.open(input_file, encoding='utf-8') as f:
        for item in FuzzyNLUFormat.parse_iter(f).items:
            sample = Sample.from_nlu_source_item(item)
            yield sample.text, UtteranceInfo(), sample


def _iterate_yt(input_table, text_col, intent_col, prev_intent_col=None, device_state_col=None, weight_col=None,
                additional_columns=None):
    for row in _yt_client.read_table(input_table):
        text = row[text_col]
        intent = row[intent_col]
        prev_intent = row[prev_intent_col] if prev_intent_col else None
        if device_state_col:
            device_state = json.loads(row[device_state_col])
        else:
            device_state = None
        weight = float(row[weight_col]) if weight_col else 1
        if additional_columns is None:
            additional_data = None
        else:
            additional_data = OrderedDict()
            for c in additional_columns:
                additional_data[c] = row[c]
        yield text, UtteranceInfo(
            true_intent=intent, prev_intent=prev_intent, device_state=device_state, weight=weight,
            additional_data=additional_data
        ), None


def iterate_file(input_file, format, **kwargs):
    if format == 'tsv':
        return _iterate_tsv(input_file, **kwargs)
    elif format == 'text':
        return _iterate_plain_text(input_file)
    elif format == 'nlu':
        return _iterate_nlu(input_file)
    elif format == 'yt':
        return _iterate_yt(input_file, **kwargs)
    else:
        raise TypeError('Unknown format %s for input file %s' % (input_file, format))
