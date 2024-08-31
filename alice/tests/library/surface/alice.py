import datetime
import logging
import uuid
from collections.abc import Iterable

import alice.tests.library.vault as vault
import alice.tests.library.uniclient as uniclient
import pytz
from alice.tests.library.directives import mm_stack_engine_get_next
from alice.tests.library.vins_response import NoneResponse, VinsResponse
from retry import retry_call


class EvoError(Exception):
    pass


class AsrRecognitionError(EvoError):
    pass


def _chain(generator):
    for _ in generator:
        if isinstance(_, Iterable):
            yield from _chain(_)
        else:
            yield _


class ExecutionSuspended(object):
    def __init__(self, till=None):
        self._condition = till

    def __call__(self):
        if not self._condition:
            return False
        return not self._condition()


class _AliceResponseIterator(object):
    def __init__(self, response_items):
        self._items = _chain(response_items)
        self._response = NoneResponse()
        self._execution_suspended = ExecutionSuspended()
        self.is_finished = False
        self.next()

    def next(self):
        if self._execution_suspended():
            return False
        for item in self._items:
            if isinstance(item, ExecutionSuspended):
                self._execution_suspended = item
                return True
            self._response = item or self._response
        self.is_finished = True
        return False

    def __getattr__(self, key):
        obj = super() if key == '_response' else self._response
        return getattr(obj, key)


class Payload(object):
    def __init__(self, application, header, **request):
        self.application = application
        self.header = header
        self.request = {k: v for k, v in request.items() if v is not None}


class Alice(object):
    def __init__(self, settings, device_state):
        self._dialog_id = None
        self._prev_req_id = None
        self._megamind_cookies = None
        self._reset_session = False
        self._voice_session = True

        self._device_state = device_state
        self._location = settings.region.location
        self._iot_config = uniclient.IoTUserInfo(settings.iot) if settings.iot else None
        self._predefined_contacts = settings.predefined_contacts

        self._response_queue = []
        self._real_time = datetime.datetime.now()

        self._application = uniclient.Application(settings.application)
        self._experiments = settings.experiments
        self._environment_state = None
        if settings.environment_state:
            self._environment_state = uniclient.EnvironmentState(settings.environment_state)

        self._alice_time = datetime.datetime.fromtimestamp(
            timestamp=int(self._application.Epoch or self._real_time.timestamp()),
            tz=pytz.timezone(settings.region.timezone),
        )
        self._application.ClientTime = self._alice_time.strftime('%Y%m%dT%H%M%S')
        self._application.Epoch = str(int(self._alice_time.timestamp()))
        self._application.Timezone = settings.region.timezone

        self._additional_options = uniclient.AdditionalOptions(
            bass_options=dict(
                client_ip=settings.region.client_ip,
                region_id=settings.region.region_id,
                user_agent=settings.user_agent,
            ),
            radiostations=settings.radio_stations,
            server_time_ms=self.alice_time_ms,
            supported_features=settings.supported_features,
            unsupported_features=settings.unsupported_features,
            permissions=settings.permissions,
        )

        logging.info(f'Vins url: {settings.vins_url}')
        logging.info(f'Uniproxy url: {settings.uniproxy_url}')
        logging.info(f'Uuid of fake-user: {self._application.Uuid}')
        # TODO: If-else is bad. Think on more general solution. Scraper mode is not an Alice's setting

        uniclient_cls = uniclient.ScraperUniclient if settings.scraper_mode else uniclient.Uniclient
        self._uniclient = retry_call(uniclient_cls, fargs=[settings, self._application], tries=3, delay=0.5)

    @property
    def application(self):
        return self._application

    @property
    def device_state(self):
        return self._device_state

    @property
    def datetime_now(self):
        return self._alice_time

    @property
    def alice_time_ms(self):
        return int(self._alice_time.timestamp() * 1000)

    @property
    def filtration_level(self):
        return self._additional_options.BassOptions.FiltrationLevel

    @property
    def supported_features(self):
        return self._additional_options.SupportedFeatures

    @property
    def unsupported_features(self):
        return self._additional_options.UnsupportedFeatures

    def _toggle_session(self):
        value = self._reset_session
        self._reset_session = False
        return value

    def reset_session(self):
        self._reset_session = True

    def login(self, username):
        self._additional_options.OAuthToken = vault.get_oauth_token(username)

    def skip(self, hours=0, minutes=0, seconds=0, milliseconds=0):
        self._update_alice_time(datetime.timedelta(
            hours=hours, minutes=minutes, seconds=seconds, milliseconds=milliseconds,
        ))
        self._update_device_state()
        for item in self._response_queue:
            while item.next():
                pass
        self._response_queue = [_ for _ in self._response_queue if not _.is_finished]

    def click(self, obj):
        return self._wrap_response(self._handle_directives(obj.directives))

    def call(self, method_name, *args, **kwargs):
        method = self.__getattribute__(method_name)
        if not method or not callable(method):
            raise EvoError(f'Alice has no method \'{method_name}\' or method is not callable.')
        return method(*args, **kwargs)

    def __call__(self, request):
        self._voice_session = not isinstance(request, str)
        return self._wrap_response(self._request(request=request))

    def _send_event(self, event):
        return self._wrap_response(self._request(event=event))

    def _wrap_response(self, response):
        response_iterator = _AliceResponseIterator(response)
        self._response_queue.append(response_iterator)
        return response_iterator

    def _request(self, request=None, event=None):
        self._update_alice_time()
        self._update_device_state()

        self._additional_options.ServerTimeMs = self.alice_time_ms
        self._application.ClientTime = self._alice_time.strftime('%Y%m%dT%H%M%S')
        self._application.Epoch = str(int(self._alice_time.timestamp()))

        return self._process_vins_response(
            self._make_request(request=request, event=event)
        )

    def _update_alice_time(self, timedelta=None):
        now = datetime.datetime.now()
        timedelta = timedelta or (now - self._real_time)
        self._real_time = now
        self._alice_time += timedelta

    def _update_device_state(self):
        super()._update_device_state()

    def _make_request(self, request=None, event=None):
        vins_response, _, asr = self._uniclient.request(
            request=request,
            payload=Payload(
                application=self._application,
                header=uniclient.SpeechKitHeader(
                    request_id=str(uuid.uuid4()),
                    prev_req_id=self._prev_req_id,
                    dialog_id=self._dialog_id,
                ),
                additional_options=self._additional_options,
                environment_state=self._environment_state,
                experiments=self._experiments,
                device_state=self._device_state,
                event=event,
                iot_config=self._iot_config,
                predefined_contacts=self._predefined_contacts,
                location=self._location,
                megamind_cookies=self._megamind_cookies,
                reset_session=self._toggle_session(),
                voice_session=self._voice_session,
            ),
        )
        if event and (event.get('ignore_answer') if hasattr(event, 'get') else event.IgnoreAnswer):
            vins_response = None

        if vins_response:
            self._prev_req_id = vins_response.directive.payload.header.request_id

        for response in asr:
            payload = response['directive']['payload']
            if payload['endOfUtt'] and len(payload['recognition']) == 1 and not payload['recognition'][0]['words']:
                raise AsrRecognitionError('ASR can\'t recognize opus and send empty hypothesis.')

        return vins_response

    def _process_vins_response(self, response):
        if not response:
            return
        response = VinsResponse(response)
        yield response
        yield self._handle_directives(response.directives)

    def _handle_directives(self, directives):
        for action in directives:
            apply_action = getattr(self, f'_apply_{action.get("type")}')
            yield apply_action(action)

    def _apply_client_action(self, action):
        return getattr(self, action.get('name'))(action.get('payload'))

    def _apply_server_action(self, action):
        if action.get('name') == mm_stack_engine_get_next:
            yield ExecutionSuspended()
        yield self._request(event=action)
