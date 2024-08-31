import json
from abc import ABC, abstractmethod
from base64 import b64encode

from google.protobuf import json_format

from alice.acceptance.modules.request_generator.lib.vins import make_event
from alice.memento.proto.api_pb2 import TRespGetAllObjects

import alice.tests.library.vault as vault

PERSONAL_DATA_KEY_USER_NAME = "/v1/personality/profile/alisa/kv/user_name"
PERSONAL_DATA_KEY_GUEST_UID = "/v1/personality/profile/alisa/kv/guest_uid"
PERSONAL_DATA_KEY_GENDER = "/v1/personality/profile/alisa/kv/gender"

PERSONAL_DATA_KEY_GUEST_USER_NAME_FORMAT = '/v1/personality/profile/alisa/kv/enrollment__{}__user_name'


class Scenario(object):
    def __init__(self, name, handle):
        self.name = name
        self.handle = handle


def _prepare_memento(memento):
    if memento is None:
        return None

    if isinstance(memento, str):
        memento = json.loads(memento)

    memento_proto = TRespGetAllObjects()
    json_format.ParseDict(memento, memento_proto)
    return b64encode(memento_proto.SerializeToString()).decode()


class ClientBiometry(object):
    def __init__(self, username, is_owner_enrolled,
                 pers_id='PersId-123',
                 personality_user_name=None):
        self.username = username
        self.is_owner_enrolled = is_owner_enrolled
        self.pers_id = pers_id if username else ''
        self.personality_user_name = personality_user_name if username else ''


class AbstractInput(ABC):
    def __init__(
        self,
        scenario=None,
        device_state=None,
        personal_data=None,
        request_patcher=None,
        memento=None,
        client_biometry=None,
    ):
        assert scenario is None or isinstance(scenario, Scenario)
        self.scenario = scenario
        self.device_state = device_state
        self.personal_data = personal_data
        self.request_patcher = request_patcher
        self.memento = _prepare_memento(memento)
        self.client_biometry = client_biometry
        self.guest_credentials = None

    @abstractmethod
    def make_event(self):
        pass

    def _lazy_load_guest_credentials(self):
        if self.guest_credentials is None and self.client_biometry and self.client_biometry.username:
            self.guest_credentials = vault.get_credentials(self.client_biometry.username)

    def make_personal_data(self):
        return self.personal_data

    def make_guest_options(self):
        if not self.client_biometry:
            return None

        if not self.client_biometry.username:
            return {
                'status': 'NoMatch',
                'is_owner_enrolled': self.client_biometry.is_owner_enrolled,
            }

        self._lazy_load_guest_credentials()
        return {
            'oauth_token': self.guest_credentials['oauth_token'],
            'yandex_uid': self.guest_credentials['uid'],
            'status': 'Match',
            'is_owner_enrolled': self.client_biometry.is_owner_enrolled,
            'pers_id': self.client_biometry.pers_id,
        }

    def make_guest_data(self):
        if not self.client_biometry or not self.client_biometry.username:
            return None
        self._lazy_load_guest_credentials()

        raw_personal_data = '{'

        if self.client_biometry.personality_user_name and self.client_biometry.pers_id:
            raw_personal_data += f'"{PERSONAL_DATA_KEY_GUEST_USER_NAME_FORMAT.format(self.client_biometry.pers_id)}":"{self.client_biometry.personality_user_name}"'

        raw_personal_data += '}'

        return {
            'user_info': {
                'uid': self.guest_credentials['uid'],
            },
            'raw_personal_data': raw_personal_data,
        }


class Text(AbstractInput):
    def __init__(self, text, scenario=None, device_state=None, personal_data=None, request_patcher=None, memento=None):
        super(Text, self).__init__(scenario, device_state, personal_data, request_patcher, memento)
        self.text = text

    def make_event(self):
        return make_event(self.text, text_source='text')


class Biometry(object):
    def __init__(
        self, is_known=True,
        known_user_id='1035351314',  # This is the uid of a 'default' user https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/python/testing/integration/conftest.py?rev=7286325#L92
        known_user_name='Боб',
        known_user_gender='male',
        guest_user_id='1234567890',
        classification=None,  # {"child":True, "female":True}
    ):
        self._is_known = is_known
        self._known_user_id = known_user_id
        self._known_user_name = known_user_name
        self._known_user_gender = known_user_gender
        self._guest_user_id = guest_user_id
        self._classification = classification or {"child": False, "female": False}

    def make_biometry_scoring(self):
        score = 0.975 if self._is_known else 0.045
        return {
            "scores_with_mode": [{
                "scores": [{
                    "score": score,
                    "user_id": self._known_user_id
                }],
                "mode": "default"
            }, {
                "scores": [{
                    "score": score,
                    "user_id": self._known_user_id
                }],
                "mode": "high_tnr"
            }, {
                "scores": [{
                    "score": score,
                    "user_id": self._known_user_id
                }],
                "mode": "high_tpr"
            }, {
                "scores": [{
                    "score": score,
                    "user_id": self._known_user_id
                }],
                "mode": "max_accuracy"
            }],
            # XXX(vitvlkv): We drop status, request_id and group_id keys here... add them if needed
            # NOTE: Scores (without mode) are obsolete
        }

    def make_biometry_classification(self):
        childScore = 0.975 if self._classification.get('child') else 0.045
        femaleScore = 0.975 if self._classification.get('female') else 0.045
        childrenName = 'child' if self._classification.get('child') else 'adult'
        genderName = 'female' if self._classification.get('female') else 'male'
        return {
            "scores": [
                {"classname": "adult", "confidence": 1.0 - childScore, "tag": "children"},
                {"classname": "child", "confidence": childScore, "tag": "children"},
                {"classname": "female", "confidence": femaleScore, "tag": "gender"},
                {"classname": "male", "confidence": 1.0 - femaleScore, "tag": "gender"}
            ],
            "simple": [
                {"classname": childrenName, "tag": "children"},
                {"classname": genderName, "tag": "gender"}
            ],
            "status": "ok"
        }

    def make_personal_data(self):
        return {
            PERSONAL_DATA_KEY_USER_NAME: self._known_user_name,
            PERSONAL_DATA_KEY_GUEST_UID: self._guest_user_id,
            PERSONAL_DATA_KEY_GENDER: self._known_user_gender,
        }


class BiometryScoringProvider(object):
    def __init__(self, request_id='d959f6d5-b7e9-41fe-bfe8-b91c08a4ba71'):
        self._request_id = request_id

    def make_biometry_scoring(self):
        return {
            "request_id": self._request_id
        }

    def make_biometry_classification(self):
        return None

    def make_personal_data(self):
        return None


class Voice(AbstractInput):
    def __init__(
        self, text,
        scenario=None,
        device_state=None,
        personal_data=None,
        request_patcher=None,
        biometry=None,
        asr_result=None,
        memento=None,
        whisper=None,
        client_biometry=None,
    ):
        super(Voice, self).__init__(scenario, device_state, personal_data, request_patcher, memento, client_biometry)
        self._text = text
        self._biometry = biometry
        self._asr_result = asr_result
        self._whisper = whisper

    def make_event(self):
        biometry_scoring = self._biometry.make_biometry_scoring() if self._biometry else None
        biometry_classification = self._biometry.make_biometry_classification() if self._biometry else None
        return make_event(
            self._text,
            text_source='voice_input',
            biometry_scoring=biometry_scoring,
            asr_result=self._asr_result,
            biometry_classification=biometry_classification,
            asr_whisper=self._whisper
        )

    def make_personal_data(self):
        super_personal_data = super(Voice, self).make_personal_data()
        self_personal_data = self._biometry.make_personal_data() if self._biometry else None
        if not super_personal_data and not self_personal_data:
            return None
        return {**(super_personal_data or {}), **(self_personal_data or {})}

    def make_guest_options(self):
        super_guest_options = super(Voice, self).make_guest_options()
        if not super_guest_options:
            return None
        super_guest_options['guest_origin'] = 'VoiceBiometry'
        return super_guest_options


class Image(AbstractInput):
    def __init__(self, img_url, capture_mode='photo'):
        super(Image, self).__init__()
        self.img_url = img_url
        self.capture_mode = capture_mode

    def make_event(self):
        return {
            'type': 'image_input',
            'payload': {
                'capture_mode': self.capture_mode,
                'img_url': self.img_url
            }
        }


class ServerAction(AbstractInput):
    def __init__(self, name, payload, is_warm_up=False, device_state=None, is_owner_enrolled=None):
        super(ServerAction, self).__init__(None, device_state)
        self.name = name
        self.payload = payload
        self.is_warm_up = is_warm_up
        self.is_owner_enrolled = is_owner_enrolled

    def make_event(self):
        return {
            'type': 'server_action',
            'name': self.name,
            'payload': self.payload,
            'is_warmup': self.is_warm_up,
        }

    def make_guest_options(self):
        if self.is_owner_enrolled is not None:
            return {
                'status': 'NoMatch',
                'is_owner_enrolled': self.is_owner_enrolled,
            }
        return None


text = Voice  # TODO(vitvlkv): Replace it with `text = Text` (in a separate PR, fix all scenarios tests)
voice = Voice
image = Image
callback = ServerAction
server_action = ServerAction
