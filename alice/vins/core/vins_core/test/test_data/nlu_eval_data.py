# coding: utf-8

import re
import operator
import pandas as pd

from collections import defaultdict, OrderedDict

from vins_core.utils.data import find_vinsfile, open_resource_file


def _add_tags_to_text(text, tags):
    text_tokens = text.split()
    tag_tokens = tags.split()
    assert len(text_tokens) == len(tag_tokens)

    data = {
        'result': '',
        'prev_tag': None,
    }

    def flush_tag():
        if data['prev_tag'] is not None:
            data['result'] += '"(%s)' % data['prev_tag']
            data['prev_tag'] = None

    def tag_name(token):
        return token[2:].lower().replace('.', '-').lstrip('@')

    for i in xrange(len(text_tokens)):
        if tag_tokens[i] == 'O' or tag_tokens[i].startswith('B-'):
            flush_tag()

        if i > 0:
            data['result'] += ' '

        if tag_tokens[i].startswith('B-'):
            data['result'] += '"'
            data['prev_tag'] = tag_name(tag_tokens[i])
        elif tag_tokens[i].startswith('I-'):
            assert data['prev_tag'] == tag_name(tag_tokens[i])
        else:
            assert tag_tokens[i] == 'O'

        data['result'] += text_tokens[i]

    flush_tag()

    return data['result']


def _read_tsv_data(filename):

    data = pd.read_csv(filename, sep='\t', encoding='utf-8', header=None, dtype=object)
    data = data.dropna()
    data[0] = data[0].apply(
        # text normalization code
        lambda text: re.sub(ur'(\.|\,)', ur'', text).lower()
    )
    if data.shape[1] == 2:
        data.columns = ['text', 'intent']
        data = data.groupby('intent').apply(lambda block: block['text'].tolist()).to_dict()
    elif data.shape[1] == 3:
        data.columns = ['text', 'tags', 'intent']
        data = data.groupby('intent').apply(
            lambda block: block.apply(
                lambda item: _add_tags_to_text(item.text, item.tags),
                axis=1
            ).tolist()
        ).to_dict()
    else:
        raise ValueError('{0}: Wrong file format'.format(filename))
    return data


def _read_simple_tsv_data(filename):
    data = defaultdict(list)

    for line in open_resource_file(filename):
        parts = line.rstrip('\n').split('\t')
        assert len(parts) == 2
        data[parts[1]].append(parts[0])

    return data


def _read_vins_config_data(appname, intent_filter=None):
    from vins_core.config.app_config import AppConfig

    print "Reading appname - %s Vinsfile.json" % appname
    app_config = AppConfig()
    app_config.parse_vinsfile(find_vinsfile(appname))

    dataset = OrderedDict()
    entities = []

    for intent in app_config.intents:
        if intent_filter and intent_filter(intent.name):
            continue

        if intent.nlu:
            for item in intent.nlu.items:
                if intent.name not in dataset:
                    dataset[intent.name] = []
                dataset[intent.name].append(item.original_text)

    dataset = OrderedDict(sorted(dataset.iteritems(), key=operator.itemgetter(0)))
    for entity in app_config.entities:
        entities.append(entity)

    return dataset, entities


def weather_data(single_intent=False):
    data = OrderedDict()
    for line in open_resource_file('vins_core/test/test_data/eval/weather.tsv'):
        parts = line.rstrip('\n').split('\t')
        utterance = parts[0].lower().strip()
        intent = parts[2].lower().strip()
        date_time = parts[3].lower().strip()
        place = parts[4].lower().strip()

        utterance = utterance.rstrip('?')

        if intent == u'' or single_intent:
            intent = u'погода'

        def mark_slot(utterance, slot_name, slot_value):
            slot_value_parts = slot_value.split(' ')
            if slot_value_parts[0] == u'в' or slot_value_parts[0] == u'на':
                slot_value = ' '.join(slot_value_parts[1:])

            if slot_value == '':
                return utterance
            if utterance.find(slot_value) == -1:
                print 'Cannot find %s in %s' % (slot_value, utterance)
                raise ValueError

            return utterance.replace(slot_value, u"'%s'(%s)" % (slot_value, slot_name))

        utterance = mark_slot(utterance, 'date_time', date_time)
        utterance = mark_slot(utterance, 'place', place)
        if intent not in data:
            data[intent] = []
        data[intent].append(utterance)

    return data


def music_data():
    data = defaultdict(list)
    with open_resource_file('vins_core/test/test_data/eval/music.tsv') as f:
        for line in f:
            parts = line.lower().rstrip('\n').split('\t')
            utterance = parts[0].strip()
            intent = parts[1].strip()
            title = parts[3].strip()
            author = parts[4].strip()
            performer = parts[5].strip()  # Алиса, Басков, ...
            genre = parts[6].strip()
            issue_date = parts[7].strip()
            source = parts[8].strip()  # откуда музыка (из фильма, игры)
            resource = parts[9].strip()  # где воспроизвести

            def mark_slot(utterance, slot_name, slot_value):
                slot_value_parts = slot_value.split(' ')
                if slot_value_parts[0] == u'в' or slot_value_parts[0] == u'на':
                    slot_value = ' '.join(slot_value_parts[1:])

                if slot_value == '':
                    return utterance
                if utterance.find(slot_value) == -1:
                    print 'Cannot find %s in %s' % (slot_value, utterance)
                    raise ValueError

                return utterance.replace(slot_value, u"'%s'(%s)" % (slot_value, slot_name))

            # utterance = mark_slot(utterance, 'category', category)
            utterance = mark_slot(utterance, 'title', title)
            utterance = mark_slot(utterance, 'author', author)
            utterance = mark_slot(utterance, 'performer', performer)
            utterance = mark_slot(utterance, 'genre', genre)
            utterance = mark_slot(utterance, 'issue_date', issue_date)
            utterance = mark_slot(utterance, 'source', source)
            utterance = mark_slot(utterance, 'resource', resource)

            if len(intent.strip()) == 0:
                intent = "unknown"

            data[intent].append(utterance)

    return data


def toy_data(n_samples=10):
    import numpy as np
    rng = np.random.RandomState(42)
    alphabets = {
        '0': list(' ab'),
        '1': list(' xy'),
        '2': list(' uv')
    }
    sentence_len = 10
    data = {
        label: map(
            lambda l: ' '.join(''.join(l).split()),
            rng.choice(letters, size=(n_samples, sentence_len))
        )
        for label, letters in alphabets.iteritems()
    }
    return data


def personal_assistant_data(intent_filter=None):
    return _read_vins_config_data('personal_assistant', intent_filter)


def navi_data():
    return _read_vins_config_data('navi_app')


def navi_flow_data(no_conditional=True):
    return _read_simple_tsv_data('vins_core/test/test_data/eval/navi_flow.tsv')


def navi_longtail_data(no_conditional=True):
    return _read_simple_tsv_data('vins_core/test/test_data/eval/navi_longtail.tsv')
