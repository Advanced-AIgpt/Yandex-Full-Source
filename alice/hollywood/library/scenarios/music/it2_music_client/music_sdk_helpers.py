import logging
import json
from urllib.parse import urlparse, parse_qs

from alice.hollywood.library.python.testing.it2.hamcrest import non_empty_dict
from hamcrest import assert_that, has_entries, contains, any_of


def get_response(response):
    '''
    Сценарий с использованием BASS возвращает ответ в continue,
    а HollywoodMusic-only может вернуть ответ в run.
    Вне зависимости от исхода, надо вернуть правильный объект ответа.
    '''
    if response.scenario_stages() == {'run', 'continue'}:
        return response.continue_response.ResponseBody
    return response.run_response.ResponseBody


def get_response_pyobj(response):
    if response.scenario_stages() == {'run', 'continue'}:
        return response.continue_response_pyobj['response']  # это не опечатка
    return response.run_response_pyobj['response_body']


def check_response(response, texts=[], output_speeches=[], with_div2_card=False):

    if with_div2_card:
        # todo: we should write better check for div2 here
        assert_that(get_response_pyobj(response), has_entries({
            'layout': has_entries({
                'cards': contains(has_entries({
                    'div2_card_extended': non_empty_dict(),
                })),
                'output_speech': any_of(*output_speeches),
            }),
            'semantic_frame': non_empty_dict(),
            'frame_actions': non_empty_dict(),
            'analytics_info': non_empty_dict(),
        }))
    else:
        assert_that(get_response_pyobj(response), has_entries({
            'layout': has_entries({
                'cards': contains(has_entries({
                    'text': any_of(*texts),
                })),
                'output_speech': any_of(*output_speeches),
            }),
            'semantic_frame': non_empty_dict(),
            'frame_actions': non_empty_dict(),
            'analytics_info': non_empty_dict(),
        }))


def check_analytics_info(response, check_obj):
    assert_that(get_response_pyobj(response), has_entries({
        'analytics_info': check_obj,
    }))


def check_suggests_common(response, has_search_suggest=False, has_recommendation_suggests=True):
    suggests = get_response(response).Layout.SuggestButtons

    # Если есть поисковой саджест (ПП), то он четвертый в списке, в остальном отличий нет
    if has_search_suggest:
        if has_recommendation_suggests:
            assert len(suggests) == 5
            sb = suggests[3].SearchButton
        else:
            assert len(suggests) == 2
            sb = suggests[0].SearchButton
        assert sb.Title
        assert sb.Title == sb.Query
    else:
        assert len(suggests) == 4

    if has_recommendation_suggests:
        frame_actions = get_response(response).FrameActions
        # По нажатию на саджест "Укупник" имитируется ввод "Включи Укупник", проверяем три саджеста
        for i in range(1, 4):
            type_text_directive = frame_actions[str(i)].Directives.List[0].TypeTextDirective
            assert type_text_directive.Text == 'Включи ' + suggests[i - 1].ActionButton.Title


def check_vins_uri(response, expected_query_info):
    vins_uri = get_response(response).Layout.Directives[0].OpenUriDirective.Uri
    uri_info = urlparse(vins_uri)
    assert uri_info.scheme == 'vins'
    assert not uri_info.path

    query_info = parse_qs(uri_info.query)
    assert query_info == expected_query_info


def check_music_sdk_uri(response, expected_query_info, tracks_count=None, first_track=None):
    music_sdk_uri = get_response(response).Layout.Directives[0].OpenUriDirective.Uri
    uri_info = urlparse(music_sdk_uri)
    assert uri_info.scheme == 'musicsdk'
    assert not uri_info.path

    query_info = parse_qs(uri_info.query)

    def fix_query_info(qi):
        qi.pop('aliceSessionId', None)  # not needed anymore

        # crop ',' at the end of 'track' (BASS bug)
        if 'track' in qi and qi['track'][0].endswith(','):
            qi['track'][0] = qi['track'][0][:-1]

        # check additional tracks without order
        if 'track' in qi and ',' in qi['track'][0]:
            track_ids = qi['track'][0].split(',')
            rest_track_ids = track_ids[1:]
            rest_track_ids.sort()
            track_ids = [track_ids[0]] + rest_track_ids

            # check first track if needed
            if first_track is not None:
                assert track_ids[0] == str(first_track)

            # check only tracks count if needed
            if tracks_count is not None:
                assert len(track_ids) == tracks_count
                del qi['track']
            else:
                qi['track'][0] = ','.join(track_ids)

    fix_query_info(query_info)
    fix_query_info(expected_query_info)

    logging.info(query_info)
    logging.info(expected_query_info)

    assert query_info == expected_query_info


def check_is_various(response, is_various):
    bass_scenario_state = json.loads(response.run_response_pyobj['continue_arguments']['scenario_args']['bass_scenario_state'])
    web_answer = bass_scenario_state['apply_arguments']['web_answer']
    assert len(web_answer['artists']) == 1
    assert web_answer['artists'][0]['is_various'] == is_various
