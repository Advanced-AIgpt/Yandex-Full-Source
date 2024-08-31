import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, HttpResponseStub, StubberEndpoint

import contact_updates


mssngr_api_registy_stubber = create_stubber_fixture(
    host='api.messenger.yandex.ru',
    port=443,
    endpoints=[
        StubberEndpoint('/api/', ['POST']),
    ],
    scheme='https',
    stubs_subdir='mssngr_api_registry',
)


def get_frozen_stubs(stub_name):
    return {
        '/api/': [
            HttpResponseStub(status_code=200, content_filename=stub_name),
        ]
    }


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'contacts'


@pytest.fixture(scope='function')
def srcrwr_params(mssngr_api_registy_stubber, kronstadt_grpc_port):
    return {
        'CONTACTS': f'localhost:{kronstadt_grpc_port}',
        'MSSNGR_API_REGISTRY_PROXY': f'http://localhost:{mssngr_api_registy_stubber.port}',
    }


@pytest.mark.scenario(name='Contacts', handle='contacts')
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments(
    'mm_enable_protocol_scenario=Contacts'
)
class TestContactsBase(object):
    pass


@pytest.mark.oauth(auth.Yandex)
class TestContacts(TestContactsBase):

    @pytest.mark.freeze_stubs(mssngr_api_registy_stubber=get_frozen_stubs('freeze_stubs/mssngr_ok_response.json'))
    @pytest.mark.device_state({
        'device_id': 'feedface-e8a2-4439-b2e7-689d95f277b7'
    })
    def test_contacts_update_request(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=contact_updates.CONTACTS_UPDATE_FRAME))
        assert response.scenario_stages() == {'run', 'apply'}

        return str(response)

    @pytest.mark.freeze_stubs(mssngr_api_registy_stubber=get_frozen_stubs('freeze_stubs/mssngr_ok_response.json'))
    @pytest.mark.device_state({
        'device_id': 'feedface-e8a2-4439-b2e7-689d95f277b7'
    })
    def test_contacts_upload_request(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=contact_updates.CONTACTS_UPLOAD_FRAME))
        assert response.scenario_stages() == {'run', 'apply'}

        return str(response)
