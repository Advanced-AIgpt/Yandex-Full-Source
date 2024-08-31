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

FORMAT = '%Y-%m-%d %H:%M:%S'
RECORDS_PER_PAGE = 25

def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('records_dir', help='path/to/parsed/wonderlogs')
    parser.add_argument('output', help='output/folder/with/htmls')
    return parser.parse_args()


def _prepare(cur_page_id, target_info):
    doc = dominate.document(title='Bio markup, page {}'.format(cur_page_id))
    with doc.head:
        tags.meta(charset='utf-8')

    all_infos = target_info
    all_infos.sort(key=lambda x: x['_server_time_ms'])

    with doc:
        with tags.form(method='post', id="the-form"):
            with tags.ul():
                tags.li('"Остальные" - люди, говорящие в колонку, но которых нет в членах семьи (забыли добавить, редко встречаются и пр.)')
                tags.li('"Не знаю" - нельзя выделить основного говорящего, речь не в колонку, топанье кота, телевизор, непонятно чья речь')
            
            with tags.table(border=1, cellspacing=2).add(tags.tbody()):
                with tags.tr():
                    tags.th('#')
                    tags.th('Client time')
                    tags.th('Audio')
                    tags.th('Name')
                    # tags.th('Comment')
                    tags.th('Text')
                    tags.th('Model')

                cur_idx = 1
                for info in all_infos:
                    # color = '#E9D9D7' if cur_idx % 2 else '#D7E8E9'
                    color = '#ffffff'
                    with tags.tr(bgcolor=color):
                        with tags.td():
                            tags.p(str(cur_idx))

                        with tags.td():
                            t_ms = datetime.fromtimestamp(info['_server_time_ms'] // 1000.0)
                            tags.span(datetime.strptime(str(t_ms), FORMAT).strftime('%d-%m-%Y %H:%M:%S'))


                        with tags.td():
                            with tags.audio(controls='controls'):
                                # CHOOSE PROPER ONE!
                                # tags.source(src=info['asr_mds_url'])
                                # tags.source(src=info['audio_url'])
                                tags.source(src=info['column2'])
                        
                        with tags.td():
                                message_id = str(info['_message_id'])
                                # with tags.select(name=f'bio_markup|{message_id}|DUMP_VALUE', onchange='this.form.submit()', id='markup'):
                                with tags.select(name=f'bio_markup|{message_id}|DUMP_VALUE', id='markup', _class='my-option'):
                                    # for cur_name in family_names:
                                    #     cur_value = '|'.join([str(cur_name), message_id, 'DUMP_VALUE'])
                                    #     tags.option(cur_name, value=cur_value, type='submit') 
                                    tags.option('Выбрать', hidden=True, selected=True)
                                    tags.p('OPTIONS_LIST')
                                    tags.option('Остальные', value='|'.join(['other', message_id, 'DUMP_VALUE']), type='submit') 
                                    tags.option('Не знаю', value='|'.join(['unknown', message_id, 'DUMP_VALUE']), type='submit') 

                        # with tags.td():
                        #     # with tags.form(method='post'):
                        #     with tags.p():
                        #         tags.label("Комментарий", fr='comment')
                        #         cur_id = '|'.join(['comment', message_id, 'DUMP_VALUE'])
                        #         tags.input_(type='text', id=cur_id, name=cur_id, autocomplete="off", style='width:30em')
                        

                        # with tags.td():
                        #     data = {
                        #         'text': info['text'],
                        #         'device_manufacturer': info['device_manufacturer'],
                        #         'device_model': info['device_model'],
                        #     }
                        #     tags.pre(json.dumps(data, indent=2, ensure_ascii=False))

                        with tags.td(align="center"):
                            tags.span(info['text'])

                        with tags.td(align="center"):
                            tags.span(info['device_model'])

                    cur_idx += 1

            tags.button('Далее', name='next', value=f'NEXT|DUMP_VALUE|{cur_page_id}', type='submit')
            # tags.button('Назад', name='prev', value=f'PREV|DUMP_VALUE|{cur_page_id}', type='submit')

            tags.button('Скачать результаты', name='dump', value='NEXT|DUMP_VALUE|{cur_page_id}', type='submit') 

            tags.p('NOT_ALL_MARKUP_WARN')

    return doc.render()


def main():
    args = _parse_args()

    records_dir = yt.list(args.records_dir)
    for cur_user in records_dir:
        cur_user_path = args.records_dir + '/' + cur_user
        cur_user_output_path = args.output + '/' + cur_user


        id2info = []
        for row in yt.read_table(cur_user_path):
            id2info.append(row)
        
        utils.make_dirs(cur_user_output_path, remove_if_exists=True)
        
        chunks_qnt = len(id2info) // RECORDS_PER_PAGE + (len(id2info) % RECORDS_PER_PAGE != 0)
        for cur_chunk in range(chunks_qnt):
            target_info = id2info[cur_chunk * RECORDS_PER_PAGE:(cur_chunk + 1) * RECORDS_PER_PAGE]            
            html = _prepare(cur_chunk, target_info)

            with open(os.path.join(cur_user_output_path, str(cur_chunk) + '.html'), 'w') as fout:
                fout.write(html)



        

