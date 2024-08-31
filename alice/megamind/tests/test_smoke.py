import pytest
import alice.megamind.tests.library.request as req
import alice.megamind.tests.library.response as resp


RESPONSE_CHANGER = [
    resp.JsonRead(),
    resp.JsonFieldRemover('response/quality_storage', 'header/response_id', 'sessions', 'version'),
    resp.JsonWrite(indent=4)
]


@pytest.mark.parametrize('test', [
    req.TestBuilder('love-tests', req.SimpleVoiceEvent('обожаю тесты', ['обожаю', 'тесты'])),
    ], ids=req.TestBuilder.idsfn)
def test_common(test, speechkit, request):
    return speechkit.run_test_request(test).ya_canonical(response_changer=RESPONSE_CHANGER)
