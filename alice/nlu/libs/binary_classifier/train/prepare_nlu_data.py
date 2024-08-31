# coding: utf-8

import argparse
import logging
import yt.wrapper as yt

from tqdm import tqdm

from vins_tools.nlu.inspection.nlu_processing_on_dataset import load_app_by_name

logger = logging.getLogger(__name__)


_SCHEMA = [
    {'name': 'intent', 'type': 'string'},
    {'name': 'utterance_text', 'type': 'string'},
    {'name': 'markup', 'type': 'string'},
    {'name': 'can_use_to_train_tagger', 'type': 'boolean'},
]


def _serialize_nlu_source_item(item):
    if '@' not in item.original_text:
        return item.original_text

    slots = sorted(item.slots, key=lambda slot: slot.begin)
    text = ''
    prev_begin_position = 0
    for slot in slots:
        text += item.text[prev_begin_position: slot.begin]
        slot_name = ('+' if slot.is_continuation else '') + slot.name
        text += u"'{}'({})".format(item.text[slot.begin: slot.end], slot_name)
        prev_begin_position = slot.end

    text += item.text[prev_begin_position:]

    return text


def _collect_data():
    app, _ = load_app_by_name(app_name='personal_assistant')
    for intent_name, data in tqdm(app.nlu.nlu_sources_data.iteritems()):
        for item in data:
            yield {
                'intent': intent_name,
                'utterance_text': item.text.encode('utf8'),
                'markup': _serialize_nlu_source_item(item),
                'can_use_to_train_tagger': item.can_use_to_train_tagger
            }


def prepare_nlu_data(table_path):
    yt.create_table(table_path, recursive=True, attributes={'schema': _SCHEMA})
    yt.write_table(table_path, _collect_data())


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--table-path', required=True)
    args = parser.parse_args()

    prepare_nlu_data(args.table_path)


if __name__ == "__main__":
    main()
