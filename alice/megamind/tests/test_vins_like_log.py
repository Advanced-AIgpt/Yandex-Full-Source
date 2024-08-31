import pytest
import alice.megamind.tests.library.request as req
import alice.megamind.tests.library.response as resp


SERVER_ACTION_PAYLOAD = {
    'caption': 'Открой сайт с анекдотами',
    'request_id': 'aa159810-4a93-4f35-8e7a-d0893e80dceb',
    'suggest_block': {
        'data': {
            'text': 'Открой сайт с анекдотами'
        },
        'suggest_type': 'from_microintent',
        'type': 'suggest'
    },
    'utterance': 'Открой сайт с анекдотами'
}

CGI_PARAMS = req.AddCGI('vins-like-log', '1')

RESPONSE_CHANGER = [
    resp.JsonRead(),
    resp.JsonFieldRemover('timestamp', 'version'),
    resp.JsonPathAction('message',
                        resp.JsonRead(),
                        resp.JsonFieldRemover('server_time_ms', 'server_time', 'analytics_info/scenario_timings', 'response_id'),
                        resp.JsonFieldRecursiveRemove('start_timestamp')),
    resp.JsonWrite(indent=4),
]


@pytest.mark.parametrize('test_req_builder', [
    req.TestBuilder('handcrafed_hello', req.VerySimpleVoiceEvent('привет'), CGI_PARAMS),
    req.TestBuilder('anekdot', req.VerySimpleVoiceEvent('расскажи анекдот'), CGI_PARAMS),
    req.TestBuilder('find_poi', req.VerySimpleVoiceEvent('кафе на улице льва толстого'), CGI_PARAMS),
    req.TestBuilder('vins_search', req.VerySimpleVoiceEvent('покажи большие сиськи'), CGI_PARAMS),
    req.TestBuilder('just_weather', req.VerySimpleVoiceEvent('погода'), CGI_PARAMS),
    req.TestBuilder('weather_in_urupinsk', req.VerySimpleVoiceEvent('погода в урюпинске'), CGI_PARAMS),
    req.TestBuilder('server_action', req.ServerActionEvent('on_suggest', SERVER_ACTION_PAYLOAD), CGI_PARAMS),
    req.TestBuilder('image_input', req.ImageInputEvent('https://avatars.mds.yandex.net/get-alice/4474472/test_YqwI1yuByZQjuT-6PESCUA/fullocr'), CGI_PARAMS),
    req.TestBuilder('text_input', req.TextInputEvent('мы писали, мы писали, наши пальчики устали'), CGI_PARAMS),
    ], ids=req.TestBuilder.idsfn)
def test_common(test_req_builder, speechkit, request):
    return speechkit.run_test_request(test_req_builder).ya_canonical(response_changer=RESPONSE_CHANGER)
