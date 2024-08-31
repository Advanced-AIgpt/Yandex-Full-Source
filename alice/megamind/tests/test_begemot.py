import pytest
import alice.megamind.tests.library.request as req
import alice.megamind.tests.library.response as resp


RESPONSE_CHANGER = [
    resp.JsonRead(),
    resp.JsonFieldRemover('duration', 'version'),
    resp.JsonWrite(indent=4)
]


@pytest.mark.parametrize('test', [
    req.TestBuilder('begemot_only_handle', req.VerySimpleVoiceEvent('приди ко мне бегемотик'), req.SetPath('begemot'), req.AddCGI('full', '1')),
    ], ids=req.TestBuilder.idsfn)
def test_common(test, speechkit, request):
    return speechkit.run_test_request(test).ya_canonical(response_changer=RESPONSE_CHANGER)
