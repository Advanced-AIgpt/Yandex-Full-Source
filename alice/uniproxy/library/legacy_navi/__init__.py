import datetime
import json
import uuid
import traceback

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, RTLogHTTPRequest
from alice.uniproxy.library.common_handlers import CommonRequestHandler
from alice.uniproxy.library.legacy_navi.navi_adapter import dont_understand_command, NaviAPIResponse
from alice.uniproxy.library.legacy_navi.navi_request import parse_request
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import UnistatTiming

from urllib.parse import unquote_plus

from rtlog import null_logger

from tornado import escape, gen, web


DONT_UNDERSTAND_TEXT = {
    'ru': 'Извините, непонятно',
    'en': 'Sorry, do not understand',
    'uk': 'Извините, не понятно',
    'tr': 'Maalesef, bu belli değil',
    'fr': 'Désolé, je ne vous ai pas compris',
}


class LegacyNaviHandler(CommonRequestHandler):
    unistat_handler_name = 'legacy_toyota'

    @gen.coroutine
    def post(self):
        logger = Logger.get('.legacy_navi')

        GlobalCounter.LEGACY_NAVI_REQUESTS_IN_SUMM.increment()

        try:
            request_data = LegacyNaviHandler._parse_json_request(self.request)
        except Exception:
            logger.exception('Failed to get request')
            raise

        try:
            uuid_argument = self.get_query_argument('uuid')
            lang = self.get_query_argument('lang')
            navi_data = request_data['navi.0.1']['data']
            default_utterance = request_data['speechkit.0.1']['data']['Utterance']
        except Exception:
            logger.exception('Failed to get (uuid, lang, data, utterance)')
            raise

        text, context = parse_request(lang, navi_data, default_utterance)

        if context.get('broken', True):
            command = dont_understand_command(DONT_UNDERSTAND_TEXT.get(context.get('lang', lang), ''))

            self.set_status(200)
            self.write({'commands': [command]})
            return

        try:
            client_ip = self.request.headers.get(
                'X-Real-Ip',
                self.request.headers.get('X-Forwarded-For', self.request.remote_ip)
            )
            user_agent = self.request.headers.get('User-Agent', '')

            vins_request_id = str(uuid.uuid4())
            now = datetime.datetime.now()
            supported_commands = context.get('commands', [])

            tl_lat = context.get('tl_lat', 0)
            tl_lon = context.get('tl_lon', 0)
            br_lat = context.get('br_lat', 0)
            br_lon = context.get('br_lon', 0)

            payload = {
                'header': {
                    'request_id': vins_request_id,
                },
                'request': {
                    'device_state': {
                        'navigator': {
                            'current_route': context.get('current_route', {}),
                            'map_view': {
                                'br_lon': br_lon,
                                'tr_lon': max(tl_lon, br_lon),
                                'tl_lon': tl_lon,
                                'bl_lon': min(tl_lon, br_lon),

                                'br_lat': br_lat,
                                'tr_lat': max(tl_lat, br_lat),
                                'tl_lat': tl_lat,
                                'bl_lat': min(tl_lat, br_lat),
                            },
                            'home': context.get('home_favourites', {}),
                            'work': context.get('work_favourites', {}),
                            'user_favorites': context.get('favourites', [])
                        },
                    },
                    'event': {
                        'text': text,
                        'type': 'text_input'
                    },
                    'location': {
                        'lat': context.get('lat', 0),
                        'lon': context.get('lon', 0),
                    }
                },
                'application': {
                    'lang': lang,
                    'uuid': uuid_argument,
                    'app_id': 'ru.yandex.mobile.navigator',

                    'timezone': 'Europe/Moscow',
                    'client_time': now.strftime("%Y%m%dT%H%M%S"),
                    'timestamp': str(int(now.timestamp())),

                    'app_version': '',
                    'os_version': '',
                    'platform': '',
                }
            }

            headers = {
                'Content-Type': 'application/json',
                'Accept': '*/*',
                'Cache-Control': self.request.headers.get('Cache-Control', 'no-cache'),
                'X-Forwarded-Host': self.request.headers.get('X-Forwarded-Host', self.request.headers.get('Host')),
                'X-Real-Ip': client_ip
            }

            if user_agent != '':
                headers['User-Agent'] = user_agent

            vins_config = config['navi']['vins']
            url = vins_config['url']
            client = QueuedHTTPClient.get_client_by_url(url)
            request = RTLogHTTPRequest(
                url,
                method='POST',
                headers=headers,
                body=json.dumps(payload).encode('utf-8'),
                request_timeout=vins_config.get('timeout', 2),
                retries=vins_config.get('retries', 3),
                rt_log=null_logger(),
                rt_log_label=vins_config.get('rt_log_label', 'get_legacy_navi'),
            )

            with UnistatTiming('legacy_navi_request_time'):
                response = yield client.fetch(request)

            parsed_response = response.json()

            logger.debug('Get response: %s' % parsed_response)

            commands = []

            cards = parsed_response.get('response', {}).get('cards', [])
            for item in parsed_response.get('response', {}).get('directives', []):
                if item.get('type') == 'client_action' and item.get('name') == 'open_uri':
                    number_card = len(commands)
                    description = cards[number_card].get('text', '') if number_card < len(cards) else ''
                    navi_response = NaviAPIResponse(item['payload']['uri'], text, description)
                    commands.append(navi_response.build_commands(supported_commands))

            if len(commands) == 0:
                command = dont_understand_command(DONT_UNDERSTAND_TEXT.get(context.get('lang', lang), ''))
                commands.append(command)

            self.set_status(200)
            self.write({'commands': commands})

            GlobalCounter.LEGACY_NAVI_REQUESTS_OK_SUMM.increment()

        except Exception:
            GlobalCounter.LEGACY_NAVI_REQUESTS_ERR_SUMM.increment()
            logger.exception('Handler failed')

            command = dont_understand_command(DONT_UNDERSTAND_TEXT.get(context.get('lang', lang), ''))

            self.set_status(200)
            self.write({'commands': [command]})

    def write_error(self, status_code, **kwargs):
        data = {
            'message': 'unhandled_error',
            'error': 'validation_error' if status_code == 400 else 'unhandled_error',
            'traceback': ''
        }

        if 'exc_info' in kwargs:
            exception = kwargs['exc_info'][1]
            exception_lines = traceback.format_exception(*kwargs['exc_info'])

            if isinstance(exception, web.HTTPError) and exception.reason:
                data['message'] = exception.reason
            elif len(exception_lines) > 0:
                data['message'] = exception_lines[-1]
            data['traceback'] = ''.join(exception_lines)

        self.write(data)

    @staticmethod
    def _parse_json_request(request):
        content_type = request.headers.get('Content-Type', '').split(';')[0]
        if content_type == 'application/x-www-form-urlencoded':
            body = unquote_plus(request.body.decode('utf-8'))
        elif content_type == 'application/json' or content_type == '':
            body = request.body.decode('utf-8')
        else:
            raise web.HTTPError(400, 'Unsupported media type "%s" in request.' % content_type)

        try:
            data = escape.json_decode(body)
        except ValueError as e:
            raise web.HTTPError(400, 'Failed to parse json %s' % e)

        # TODO(ikorobtsev): add schema validation
        # if schema:
        #     try:
        #         validate_json(data, schema)
        #     except SchemaValidationError as e:
        #         raise ValidationError(unicode(e), params={'data': data})

        return data
