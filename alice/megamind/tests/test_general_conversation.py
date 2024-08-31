import pytest
import alice.megamind.tests.library.request as req
import alice.megamind.tests.library.response as resp


RESPONSE_CHANGER = [
    resp.JsonRead(),
    resp.JsonFieldRemover('response/quality_storage', 'sessions', 'header/response_id', 'response/suggest', 'version'),
    resp.JsonWrite(indent=4)
]


@pytest.mark.parametrize('test', [
    req.TestBuilder('force_protocol', req.VerySimpleVoiceEvent('что ты думаешь про котиков?'), req.EnableExpFlag('mm_scenario=GeneralConversation')),
    req.TestBuilder('enable_protocol', req.VerySimpleVoiceEvent('что ты думаешь про котиков?')),
    req.TestBuilder('enable_protocol__pure_gc__vins', req.VerySimpleVoiceEvent('Давай поболтаем?')),
    req.TestBuilder(
        'enable_protocol__pure_gc__protocol',
        req.VerySimpleVoiceEvent('Давай поболтаем?'),
        req.EnableExpFlag('mm_enable_protocol_scenario=GeneralConversation'),
        req.EnableExpFlag('mm_gc_pure_protocol'))
    ], ids=req.TestBuilder.idsfn)
def test_common(test, speechkit, request):
    return speechkit.run_test_request(test).ya_canonical(response_changer=RESPONSE_CHANGER)
