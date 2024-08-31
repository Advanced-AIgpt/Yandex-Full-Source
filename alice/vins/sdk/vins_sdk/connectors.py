# coding: utf-8
from __future__ import unicode_literals

import io
import json
import logging
from collections import defaultdict
from copy import deepcopy
from threading import Lock
from uuid import uuid4

from telegram import (
    ChatAction,
    KeyboardButton, ReplyKeyboardMarkup,
    Bot,
    InlineKeyboardButton, InlineKeyboardMarkup,
)
from telegram.ext import Updater, MessageHandler, CallbackQueryHandler
from telegram.replykeyboardremove import ReplyKeyboardRemove
from telegram.utils.request import Request

from vins_core.dm.request import AppInfo, ReqInfo, Experiments
from vins_core.dm.request_events import ServerActionEvent, TextInputEvent, SuggestedInputEvent
from vins_core.dm.response import ClientActionDirective, ServerActionDirective
from vins_core.ext.voice_synth import VoiceSynthAPI
from vins_core.common.utterance import Utterance
from vins_core.utils.datetime import utcnow
from vins_core.utils.strings import smart_unicode
from vins_core.utils.misc import gen_uuid_for_tests

logger = logging.getLogger(__name__)

YANDEX_OFFICE_LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}


FIXLIST = [
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': 'погода на марсе',
    },
    {
        'intent': 'personal_assistant.internal.hardcoded',
        'text': 'погода в кандалакше',
    },
    {
        'intent': 'personal_assistant.scenarios.external_skill',
        'text': 'слушай да давай поболтаем',
    },
]

BANLIST = [
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': r'.*\bавада кедавра\b.*'
    },
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': '.*мусульман.*'
    },
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': '.*коран.*'
    },
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': '.*католи[кч].*'
    },
    {
        'intent': 'personal_assistant.general_conversation.general_conversation_dummy',
        'text': '.*евреев.*'
    }
]

HARDCODED_RESPONSES = {
    'resp1': {
        'regexps': [
            'ответь заготовленной репликой'
        ],
        'responses': [
            'Ок, отвечаю!'
        ]
    },
    'resp2': {
        'regexps': [
            '.*собаченька.*'
        ],
        'responses': [
            'Гав!'
        ]
    },
    'resp3': {
        'regexps': [
            'мне нужен ответ с переводом строки'
        ],
        'responses': [
            'Лови\nДержи'
        ]
    },
    'resp4': {
        'regexps': [
            'мне нужен ответ текстом и голосом',
            'лажовая [ регулярка'
        ],
        'responses': [
            {
                'voice': 'держи голос',
                'text': 'держи текст'
            },
            {
                'key': 'is wrong'
            },
            22436278
        ]
    },
    'resp5': {
        'regexps': [
            'мне нужен ответ только текстом',
            {
                'dict': 'instead of string'
            }
        ],
        'responses': [
            {
                'voice': '',
                'text': 'у нас так не принято'
            }
        ]
    },
    'resp6': {
        'regexps': [
            'мне нужен ответ только голосом'
        ],
        'responses': [
            {
                'voice': 'у нас так не принято',
                'text': ''
            }
        ]
    },
    'resp7': {
        'regexps': [
            'мне нужны ссылки'
        ],
        'responses': [
            'вот ответ'
        ],
        'links': [
            {
                'title': 'яндекс',
                'url': 'http://yandex.ru/'
            },
            {
            }
        ]
    },
    'resp8': {
        'regexps': [
            'мне нужны ссылки'
        ],
        'responses': [
            'вот ответ'
        ],
        'links': [123, "456"]
    },
    'resp9': {
        'regexps': [
            'мне нужны ссылки'
        ],
        'responses': [
            'вот ответ'
        ],
        'links': [
            {
                'url': 'http://yandex.ru/',
            },
            {
                'url': 'not_a_valid_url',
                'title': 'ttt'
            },
            {
                'title': 'tt',
            }
        ]
    },
    'resp10': {
        'app_id': 'ru.yandex.quasar.*',
        'regexps': [
            'отвечай только если ты станция'
        ],
        'responses': [{
            'voice': 'привет я станция',
            'text': 'Привет, я станция!'
        }]
    },
    'resp11': {
        'app_id': 'какая то лажа [',
        'regexps': [
            'отвечай только если ты станция'
        ],
        'responses': [
            'этого не должно случиться'
        ]
    },
    'resp12': {
        'app_id': 'ru.yandex.quasar',
        'regexps': [
            'отвечай только если ты станция'
        ],
        'responses': [
            'этого не должно случиться'
        ]
    }
}

GC_BANLIST = [
    '.*котик.*',
]


class ConnectorBase(object):

    def __init__(self, vins_app=None, req_info_args=None, **kwargs):
        if not vins_app:
            raise RuntimeError('vins_app is required')
        self._vins_app = vins_app
        self._req_info_kwargs = req_info_args or {}

    @property
    def vins_app(self):
        return self._vins_app

    def handle_request(self, req_info, **kwargs):
        return self._vins_app.handle_request(req_info=req_info)


class TelegramConnector(ConnectorBase):
    @classmethod
    def _to_action_button_text(cls, text):
        """
        Added zero length space symbol to distinguish it from user utterance
        """
        return '\u200b'.join(text)

    def _is_action_button_text(self, text, chat_id):
        if len(text) == 0:
            return False
        elif len(text) == 1:
            return text in self._button_directives.get(chat_id, {})
        else:
            return all([ch == '\u200b' for ch in text[1::2]])

    def _make_uuid(self, chat_id):
        if self._test_user is None:
            return str(chat_id).zfill(32)
        return self._test_user.uuid

    def _make_req_info_kwargs(self):
        if self._test_user is None:
            return self._req_info_kwargs
        req_info_kwargs = self._req_info_kwargs.copy()
        additional_options = req_info_kwargs.setdefault('additional_options', {})
        additional_options['test_user_oauth_token'] = self._test_user.token
        return req_info_kwargs

    @classmethod
    def _normalize_action_button_text(cls, text):
        """
        the inverse transform of cls._to_action_button_text
        """
        return text[::2]

    @classmethod
    def _error(cls, bot, update, error):
        """
        using for handle telegram dispatcher errors
        """
        logger.error('Update "%s" caused error "%s"', update, error)

    def __init__(self, bot_token, commands_map=None, app_id=None, platform=None, **kwargs):
        super(TelegramConnector, self).__init__(**kwargs)
        self._bot = Bot(bot_token, request=Request(con_pool_size=100))
        self._updater = Updater(bot=self._bot, workers=1)
        self._button_directives = defaultdict(dict)
        self._commands_map = commands_map or {
            '/start': 'привет',
        }
        self.voice_synth_api = VoiceSynthAPI()
        self.voice_status_per_user = {}
        self._activated_experiments = Experiments()
        self.switched_on_experiments_per_user = {}
        self.user_lock = Lock()
        self._req_info_kwargs['app_info'] = AppInfo(
            app_id=app_id or 'telegram',
            app_version='10.00',
            os_version='5.1.1',
            platform=platform or 'telegram',
        )
        self._req_info_kwargs['location'] = YANDEX_OFFICE_LOCATION
        self._test_user = None

    def set_voice_status(self, uuid, status):
        with self.user_lock:
            self.voice_status_per_user[uuid] = status
        self._bot.send_message(uuid, text=str('Voice is %s' % ('on' if status else 'off')))

    def get_voice_status(self, uuid):
        with self.user_lock:
            return self.voice_status_per_user.get(uuid, False)

    def switch_experiment(self, uuid, experiment_name):
        with self.user_lock:
            switched_on_experiments = self.switched_on_experiments_per_user.get(uuid)

            if not switched_on_experiments:
                switched_on_experiments = {}
                self.switched_on_experiments_per_user[uuid] = switched_on_experiments

            if (experiment_name in switched_on_experiments and
                    switched_on_experiments[experiment_name] == Experiments.ENABLED_VALUE):
                switched_on_experiments[experiment_name] = None
                return False
            else:
                switched_on_experiments[experiment_name] = Experiments.ENABLED_VALUE
                return True

    def get_experiments(self, uuid):
        return self._activated_experiments.merge(self.switched_on_experiments_per_user.get(uuid, {}))

    def _inline_button_handle(self, bot, update):
        key = update.callback_query.data
        button_directives = self._button_directives.get(update.callback_query.message.chat_id, {}).get(key)
        self._process_button_directives(update.callback_query.message, button_directives)

    def _suggest_button_handle(self, message, utterance):
        logger.info('Handle suggest button: %r', utterance)
        button_directives = self._button_directives.get(message.chat_id, {}).get(utterance)
        self._process_button_directives(message, button_directives)

    def _process_button_directives(self, message, button_directives):
        if not button_directives:
            return

        for directive in button_directives:
            if isinstance(directive, ClientActionDirective):
                logger.info('Handle client callback: %s', directive.name)
                text_to_send = None
                if directive.name == 'type':
                    event = SuggestedInputEvent(directive.payload['text'])
                    response = self._vins_app.handle_request(ReqInfo(
                        uuid=self._make_uuid(message.chat_id),
                        client_time=message.date,
                        utterance=event.utterance,
                        event=event,
                        experiments=self.get_experiments(message.chat_id),
                        **self._make_req_info_kwargs()
                    ))
                    self._handle_response(message.chat_id, response)
                elif directive.name == 'type_silent':
                    text_to_send = 'User says: %s' % directive.payload['text']
                elif directive.name == 'open_uri':
                    text_to_send = 'URI: %s' % directive.payload['uri']
                else:
                    text_to_send = 'Unsupported directive: %s' % directive.name

                if text_to_send:
                    self._send_message(message.chat_id, text_to_send)
            elif isinstance(directive, ServerActionDirective):
                logger.info('Handle server callback: %s', directive.name)
                response = self._vins_app.handle_request(ReqInfo(
                    uuid=self._make_uuid(message.chat_id),
                    client_time=message.date,
                    event=ServerActionEvent(name=directive.name, payload=directive.payload),
                    experiments=self.get_experiments(message.chat_id),
                    **self._make_req_info_kwargs()
                ))
                self._handle_response(message.chat_id, response)

    def _send_message(self, chat_id, text, buttons=None, **kwargs):
        logger.info('Send message: %s', text)
        self._bot.sendMessage(
            chat_id, text=text,
            reply_markup=ReplyKeyboardMarkup(
                [[b] for b in buttons],
                one_time_keyboard=True,
            ) if buttons else ReplyKeyboardRemove())

    def _send_markdown_message(self, chat_id, text, buttons=None, **kwargs):
        logger.info('Send markdown message: %s', text)
        self._bot.sendMessage(
            chat_id, text=text, parse_mode='Markdown',
            reply_markup=ReplyKeyboardMarkup(
                [[b] for b in buttons],
                one_time_keyboard=True,
            ) if buttons else ReplyKeyboardRemove())

    def _make_button(self, chat_id, btn):
        self._button_directives[chat_id][btn.title] = btn.directives
        button_text = self._to_action_button_text(btn.title)
        return KeyboardButton(text=button_text)

    def _make_inline_button(self, chat_id, btn):
        key = str(uuid4())
        self._button_directives[chat_id][key] = btn.directives
        return InlineKeyboardButton(btn.title, callback_data=key)

    def _handle_response(self, chat_id, response, parse_mode=None):
        suggest_buttons = [[self._make_button(chat_id, b)] for b in response.suggests]
        suggest_buttons_markup = ReplyKeyboardMarkup(
            suggest_buttons,
            one_time_keyboard=True,
        ) if suggest_buttons else ReplyKeyboardRemove()

        voice_status = self.get_voice_status(chat_id)

        if voice_status and response.voice_text:
            r = self.voice_synth_api.submit(response.voice_text)
            binary_file = io.BytesIO(r.content)
            binary_file.name = str(uuid4()) + str('.mp3')
            self._bot.send_voice(chat_id=chat_id, voice=binary_file)

        for card in response.cards:
            self._bot.sendMessage(chat_id, text=card.text, parse_mode=parse_mode, reply_markup=suggest_buttons_markup)

            if card.type == 'text_with_button':
                inline_buttons_markup = InlineKeyboardMarkup(
                    [[self._make_inline_button(chat_id, b)] for b in card.buttons],
                )
                self._bot.sendMessage(
                    chat_id, text='Actions:', parse_mode=parse_mode, reply_markup=inline_buttons_markup
                )

        for directive in response.directives:
            if isinstance(directive, ClientActionDirective):
                if directive.name == 'open_uri':
                    self._bot.sendMessage(
                        chat_id, text=directive.payload['uri'], parse_mode=parse_mode,
                        reply_markup=suggest_buttons_markup
                    )
                else:
                    directive_dump = json.dumps(directive.to_dict(), ensure_ascii=False, indent=2)
                    self._bot.sendMessage(
                        chat_id, text=directive_dump, parse_mode=parse_mode, reply_markup=suggest_buttons_markup
                    )

    def _change_app_id(self, chat_id, client):
        client_info = client.split()
        client = client_info[0]
        app_version = '10.00'
        os_version = '5.1.1'
        if len(client_info) > 1:
            app_version = client_info[1]
        if len(client_info) > 2:
            os_version = client_info[2]
        if client == 'telegram':
            app_id, platform = 'telegram', 'telegram'
        elif client == 'android':
            app_id, platform = 'ru.yandex.searchplugin', 'android'
        elif client == 'ios':
            app_id, platform = 'ru.yandex.mobile', 'iphone'
        elif client == 'windows':
            app_id, platform = 'winsearchbar', 'windows'
        elif client == 'quasar':
            app_id, platform = 'ru.yandex.quasar.vins_telegram_client', 'android'
        elif client == 'navi':
            app_id, platform = 'ru.yandex.mobile.navigator.vins_telegram_client', 'android'
        elif client == 'elari':
            app_id, platform = 'ru.yandex.iosdk.elariwatch', 'android'
        elif client == 'auto':
            app_id, platform = 'ru.yandex.auto', 'android'
        else:
            app_id_platform = client.split(',', 1)
            if len(app_id_platform) < 2:
                logger.error('Unable to parse %s, should be of "app_id,platform" format' % client)
                return
            app_id, platform = app_id_platform

        self._req_info_kwargs['app_info'] = AppInfo(
            app_id=app_id,
            app_version=app_version,
            os_version=os_version,
            platform=platform,
        )

        self._bot.sendMessage(chat_id, 'Client changed to app_id=%s, platform=%s' % (app_id, platform))

    def _change_device_state(self, chat_id, device_state_str):
        try:
            device_state = json.loads(device_state_str)
        except ValueError as exc:
            self._bot.sendMessage(chat_id, 'Incorrect json string for device state: %s' % exc.message)
        else:
            self._req_info_kwargs['device_state'] = device_state
            self._bot.sendMessage(chat_id, 'Device_state has changed')

    def _change_location(self, chat_id, location):
        try:
            lat, lon = map(float, location.split())
        except ValueError:
            self._bot.sendMessage(
                chat_id,
                'Bad location format. Expected format is (without quotes): "latitude longitude".'
            )
        else:
            self._req_info_kwargs['location'] = {'lon': lon, 'lat': lat}
            self._bot.sendMessage(chat_id, 'Location is set to (lat: %.6f, lon: %.6f)' % (lat, lon))

    def _switch_auth(self, chat_id, tags_list):
        """
        :param chat_id: telegram chat id
        :param tags_list: user tag list (e.g. ['has_home'], ['has_home', 'has_work'])
        :return: None
        """
        if self._test_user is not None:
            self._test_user = None
            self._bot.sendMessage(chat_id, 'Authentication disabled')
            return

        if not hasattr(self.vins_app, 'get_api'):
            self._bot.sendMessage(chat_id, 'Current app doesn\'t support authentication')
            return

        try:
            self._test_user = self.vins_app.get_api().get_test_user(tags_list=tags_list)
        except Exception as e:
            self._bot.sendMessage(chat_id, 'Failed to authenticate as test user: {0!r}'.format(e))
            return

        tag_list_msg = ''
        if tags_list:
            tag_list_msg = ' with tags: ' + ', '.join(map(lambda x: '"{0}"'.format(x), tags_list))
        self._bot.sendMessage(chat_id, 'Authenticated as test user' + tag_list_msg)

    def _bot_handler(self, bot, update):
        utterance = update.message.text
        logger.info('Message received: %r', utterance)
        self._bot.sendChatAction(update.message.chat_id, ChatAction.TYPING)
        if self._is_action_button_text(utterance, update.message.chat_id):
            utterance = self._normalize_action_button_text(utterance)
            self._suggest_button_handle(update.message, utterance)
        else:
            if utterance == '/voice':
                self.set_voice_status(update.message.chat_id, True)
            elif utterance == '/mute':
                self.set_voice_status(update.message.chat_id, False)
            elif utterance.startswith('/client '):
                self._change_app_id(update.message.chat_id, utterance.split()[1])
            elif utterance.startswith('/experiment '):
                experiment_name = utterance[11:].strip()
                status = self.switch_experiment(update.message.chat_id, experiment_name)
                self._bot.sendMessage(
                    update.message.chat_id,
                    text=str('Experiment %s switched %s' % (experiment_name, 'on' if status else 'off'))
                )
            elif utterance.startswith('/device_state '):
                device_state = utterance.split('/device_state ', 1)[1]
                self._change_device_state(update.message.chat_id, device_state)
            elif utterance.startswith('/location '):
                location = utterance.split('/location ', 1)[1]
                self._change_location(update.message.chat_id, location)
            elif utterance.startswith('/auth'):
                tags_list = utterance.split()[1:]
                self._switch_auth(update.message.chat_id, tags_list)
            elif utterance.startswith('/callback '):
                # /callback on_reset_session mode=onboarding
                parts = utterance.split()
                callback_name, callback_pairs = parts[1], parts[2:]
                callback_args = {}
                for pair in callback_pairs:
                    key, value = pair.split('=', 1)
                    try:
                        value = json.loads(value)
                    except Exception:
                        pass

                    callback_args[key] = value

                event = ServerActionEvent(name=callback_name, payload=callback_args)
                req_info = ReqInfo(
                    uuid=self._make_uuid(update.message.chat_id),
                    client_time=update.message.date,
                    event=event,
                    experiments=self.get_experiments(update.message.chat_id),
                    **self._make_req_info_kwargs()
                )
                response = self._vins_app.handle_request(req_info)
                self._handle_response(update.message.chat_id, response)
            else:
                if utterance.startswith('/') and utterance in self._commands_map:
                    utterance = self._commands_map[utterance]
                req_info = ReqInfo(
                    uuid=self._make_uuid(update.message.chat_id),
                    utterance=Utterance(utterance),
                    client_time=update.message.date,
                    event=TextInputEvent(utterance),
                    experiments=self.get_experiments(update.message.chat_id),
                    **self._make_req_info_kwargs()
                )

                req_info.additional_options['from_username'] = update.message.from_user.username
                forward_from = update.message.forward_from.username if update.message.forward_from else None
                req_info.additional_options['forward_from'] = forward_from
                contact_info = update.message.contact
                req_info.additional_options['contact_info'] = contact_info.to_dict() if contact_info else None

                response = self._vins_app.handle_request(req_info)
                self._handle_response(update.message.chat_id, response)

    def run(self):
        # Get the dispatcher to register handlers
        dp = self._updater.dispatcher
        # on noncommand i.e message
        dp.add_handler(CallbackQueryHandler(self._inline_button_handle))
        dp.add_handler(MessageHandler([lambda x: True], self._bot_handler))
        # log all errors
        dp.add_error_handler(self._error)
        # Start the Bot
        self._updater.start_polling()
        # Run the bot until the you presses Ctrl-C or the process receives SIGINT,
        # SIGTERM or SIGABRT. This should be used most of the time, since
        # start_polling() is non-blocking and will stop the bot gracefully.
        self._updater.idle()


class TestConnector(ConnectorBase):
    """
    Class connector for test vins application.
    """
    def __init__(self, **kwargs):
        super(TestConnector, self).__init__(**kwargs)
        self._req_info_kwargs['app_info'] = AppInfo(
            app_id='com.yandex.vins.tests',
            app_version='0.0.1',
            os_version='1',
            platform='unknown',
        )

        self._req_info_kwargs['location'] = YANDEX_OFFICE_LOCATION

    def handle_callback(self, uuid, callback_name, callback_args=None, experiments=None, text_only=True, **kwargs):
        req_info = ReqInfo(
            uuid=uuid,
            event=ServerActionEvent(name=callback_name, payload=callback_args),
            client_time=utcnow(),
            experiments=Experiments(experiments),
            **self._req_info_kwargs
        )
        response = self._vins_app.handle_request(req_info, **kwargs)
        return self._make_response(response, text_only)

    def handle_reqinfo(self, req_info, **kwargs):
        response = self._vins_app.handle_request(req_info, **kwargs)
        return response.to_dict()

    def prepare_req_info(
        self, uuid, event, experiments=None, reset_session=False,
        location=YANDEX_OFFICE_LOCATION, app_info=None, device_state=None,
        additional_options=None, dialog_id=None, client_time=None, ensure_purity=False,
        request_id=None, session=None,
    ):
        req_info_common = deepcopy(self._req_info_kwargs)
        req_info_common['location'] = location
        if app_info:
            req_info_common['app_info'] = app_info

        if request_id is None:
            request_id = str(gen_uuid_for_tests())
        req_info = ReqInfo(
            request_id=request_id,
            uuid=uuid,
            utterance=event.utterance,
            event=event,
            reset_session=reset_session,
            client_time=client_time or utcnow(),
            experiments=Experiments(experiments),
            device_state=device_state or {},
            additional_options=additional_options or {},
            dialog_id=dialog_id,
            ensure_purity=ensure_purity,
            session=session,
            **req_info_common
        )
        return req_info

    def handle_event(
        self, uuid, event, experiments=None, text_only=True, reset_session=False,
        location=YANDEX_OFFICE_LOCATION, app_info=None, device_state=None,
        additional_options=None, dialog_id=None, client_time=None, ensure_purity=False,
        request_id=None, session=None,
        **kwargs
    ):

        req_info = self.prepare_req_info(
            uuid, event, experiments, reset_session,
            location, app_info, device_state,
            additional_options, dialog_id, client_time,
            request_id=request_id, session=session,
            ensure_purity=ensure_purity,
        )

        logger.debug('TestConnector: handling request %s' % str(req_info))

        response = self._vins_app.handle_request(req_info, **kwargs)
        return self._make_response(response, text_only)

    def handle_semantic_frames(
        self, uuid, utterance, experiments=None, reset_session=False,
        location=YANDEX_OFFICE_LOCATION, app_info=None, device_state=None,
        additional_options=None, dialog_id=None, client_time=None, **kwargs
    ):
        event = TextInputEvent(utterance)
        req_info = self.prepare_req_info(
            uuid, event, experiments, reset_session,
            location, app_info, device_state,
            additional_options, dialog_id, client_time)

        logger.debug('TestConnector: handling request %s' % str(req_info))

        return self._vins_app.handle_semantic_frames(req_info, **kwargs)

    def handle_utterance(
        self, uuid, utterance, experiments=None, text_only=True, reset_session=False,
        location=YANDEX_OFFICE_LOCATION, app_info=None, device_state=None, client_time=None,
        additional_options=None, ensure_purity=False, request_id=None, **kwargs
    ):
        """
        Handle utterance and return answers and/or actions

        Join all messages in one string.
        Return string if text_only is True (default), else return dictionary with text, actions and suggest

        Parameters:
        -----------
        uuid: uuid,
        utterance: client utterance, str, unicode or Utterance object
        experiments: list of experiment flags
        text_only: True/False
        location: user location
        app_info: application info, containing application id
        device_state: device state
        """

        if isinstance(utterance, basestring):
            utterance = Utterance(smart_unicode(utterance))

        return self.handle_event(
            uuid,
            TextInputEvent(utterance.text),
            experiments=experiments,
            text_only=text_only,
            reset_session=reset_session,
            location=location,
            app_info=app_info,
            device_state=device_state,
            additional_options=additional_options or {},
            client_time=client_time,
            ensure_purity=ensure_purity,
            request_id=request_id,
            **kwargs
        )

    def get_default_reqinfo_kwargs(self):
        return self._req_info_kwargs

    @staticmethod
    def _make_response(response, text_only):
        if text_only:
            return '\n'.join(c.text for c in response.cards)
        else:
            return response.to_dict()
