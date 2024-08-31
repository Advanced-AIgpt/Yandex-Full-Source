import argparse
import os
from typing import List
import json
from datetime import datetime

import yt.wrapper as yt

import dominate
import dominate.tags as tags

import voicetech.common.lib.utils as utils

logger = utils.initialize_logging(__name__)

FORMAT = '%Y%m%dT%H%M%S'


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--brief', action='store_true')
    parser.add_argument('context_table', help='//path/to/context/table')
    parser.add_argument('info_table', help='path/to/parsed/wonderlogs')
    parser.add_argument('output', help='output/folder/with/htmls')
    return parser.parse_args()


@yt.yt_dataclass
class ContextRow:
    _message_id: str
    context_message_ids: List[str]


def _dump(args):
    id2context = {}
    for row in yt.read_table_structured(args.context_table, ContextRow):
        id2context[row._message_id] = row.context_message_ids
    with open('context.json', 'w') as fout:
        json.dump(id2context, fout, ensure_ascii=False)

    id2info = {}
    for row in yt.read_table(args.info_table):
        id2info[row['_message_id']] = row
    with open('info.json', 'w') as fout:
        json.dump(id2info, fout, ensure_ascii=False)


def _prepare(id, target_info, context_infos, brief):
    doc = dominate.document(title='Brbr markup for {}'.format(id))
    with doc.head:
        tags.meta(charset='utf-8')

    all_infos = context_infos + [target_info]
    all_infos.sort(key=lambda x: x['client_time'])

    with doc:
        with tags.table(border=1, cellspacing=2).add(tags.tbody()):
            with tags.tr():
                tags.th('Client time')
                tags.th('ASR')
                tags.th('Spotter')
                if not brief:
                    tags.th('Response cards')
                    tags.th('Response directives')
                tags.th('Info')

            for info in all_infos:
                is_target = info['_message_id'] == id
                color = '#aaaaaa' if is_target else '#ffffff'
                exclude_elem = ([], [None], [None, None])
                check_elem = ([None, ['tts_play_placeholder tts_play_placeholder']], [None, ['draw_scled_animations draw_segment_animations'], ['tts_play_placeholder tts_play_placeholder']])
                with tags.tr(bgcolor=color):
                    with tags.td():
                        tags.span(datetime.strptime(info['client_time'], FORMAT).strftime('%d-%m-%Y %H:%M:%S'))

                    with tags.td():
                        with tags.audio(controls='controls'):
                            tags.source(src=info['asr_mds_url'])

                    with tags.td():
                        if info['spotter_mds_url']:
                            with tags.audio(controls='controls'):
                                tags.source(src=info['spotter_mds_url'])
                        else:
                            tags.span('N/A')
                    if not brief:
                        with tags.td():
                            if info['response_cards'] is not None and info['response_cards'] != []:
                                cards = '\n'.join(x for x in info['response_cards'][0] if x is not None)
                                tags.pre(cards)
                            else:
                                tags.span('N/A')
                        with tags.td():
                            if info['response_directives'] in check_elem:
                                dirs = '\n'.join(x[0] for x in info['response_directives'][1:] if x is not None)
                                tags.pre(dirs)
                            elif info['response_directives'] is not None and \
                                    info['response_directives'] not in exclude_elem:
                                dirs = '\n'.join(x for x in info['response_directives'][0] if x is not None)
                                tags.pre(dirs)
                            else:
                                tags.span('N/A')
                    with tags.td():
                        if brief:
                            data = {
                                'activation': info['activation_type'],
                                'topic': info['topic'],
                                'device': info['device_model'],
                            }
                        else:
                            data = {
                                'activation': info['activation_type'],
                                'topic': info['topic'],
                                'device': info['device_model'],
                                'last track': info['current_music_track'],
                                'alarm playing': info['alarm_playing'],
                                'asr': info['asr_text'],
                            }
                        tags.pre(json.dumps(data, indent=2, ensure_ascii=False))

        with tags.form(method='post'):
            with tags.p():
                tags.label("Комментарий", fr='comment')
                tags.input_(type='text', id='comment', name='comment', autocomplete="off", style='width:30em')
            with tags.p():
                tags.label("Бырбыр", fr='brbr')
                tags.input_(type='checkbox', id='brbr', name='brbr')
            with tags.p():
                tags.label("Сайдспич", fr='sidespeech')
                tags.input_(type='checkbox', id='sidespeech', name='sidespeech')
            with tags.p():
                tags.label("Контекст понадобился?", fr='context')
                tags.input_(type='checkbox', id='context', name='context')
            tags.button('Полезно', name='markup', value='USEFUL_VALUE', type='submit')
            tags.button('Не полезно', name='markup', value='USELESS_VALUE', type='submit')

        with tags.form(method='post'):
            tags.button('Скачать результаты', name='dump', value='DUMP_VALUE', type='submit')

    return doc.render()


def main():
    args = _parse_args()

    id2context = {}
    for row in yt.read_table_structured(args.context_table, ContextRow):
        id2context[row._message_id] = row.context_message_ids

    id2info = {}
    for row in yt.read_table(args.info_table):
        id2info[row['_message_id']] = row

    utils.make_dirs(args.output, remove_if_exists=True)
    for id, context in id2context.items():
        if id not in id2info:
            logger.warning("Didn't find target id in info (id=%s)", id)
            continue
        target_info = id2info[id]
        context_infos = []
        all_found = True
        for context_id in context:
            if context_id not in id2info:
                logger.warning("Didn't find context id in info (id=%s)", id)
                all_found = False
                break

            context_infos.append(id2info[context_id])
        if not all_found:
            continue

        html = _prepare(id, target_info, context_infos, args.brief)
        with open(os.path.join(args.output, id + '.html'), 'w') as fout:
            fout.write(html)
