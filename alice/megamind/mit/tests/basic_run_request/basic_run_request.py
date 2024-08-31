from alice.megamind.mit.library.request_builder import Voice


class TestBasicRunRequest(object):
    def test_basic_run_request(self, alice):
        response = alice(Voice('рандомное число от 1 до 1000?'))
        return response.card
