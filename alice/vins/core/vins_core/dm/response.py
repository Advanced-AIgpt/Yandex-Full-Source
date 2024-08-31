# coding: utf-8

from __future__ import unicode_literals


import attr
import base64
import json
import logging

from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from alice.megamind.protos.scenarios.response_pb2 import TFrameAction

from vins_core.utils.mixins import ToDictMixin

from google.protobuf.json_format import MessageToJson


logger = logging.getLogger(__name__)

_meta_classes = {}
_special_button_classes = {}


def register_meta(cls, type_):
    _meta_classes[type_] = cls


def get_meta(type_):
    return _meta_classes[type_]


def has_meta(type_):
    return type_ in _meta_classes


def register_special_button(cls, type_):
    _special_button_classes[type_] = cls


def get_special_button(type_):
    return _special_button_classes[type_]


def has_special_button(type_):
    return type_ in _special_button_classes


_features_classes = {}


def register_feature(cls):
    _features_classes[cls.FEATURE_TYPE] = cls


def _get_feature_cls(type_):
    return _features_classes[type_]


class Card(object):
    @staticmethod
    def from_dict(data):
        if data.get('type') == 'simple_text':
            return SimpleCard(text=data['text'], tag=data.get('tag'))
        elif data.get('type') == 'text_with_button':
            return CardWithButtons(
                text=data['text'],
                buttons=[Button.from_dict(b) for b in data['buttons']]
            )
        elif data.get('type') == 'div_card':
            return DivCard(
                body=data['body'],
                text=data['text']
            )
        elif data.get('type') == 'div2_card':
            return Div2Card(
                body=data['body'],
                hide_borders=data.get('hide_borders'),
                text=data.get('text', None),
            )
        else:
            raise ValueError("Wrong Card type '%s'", data.get('type'))


@attr.s
class SimpleCard(ToDictMixin):
    text = attr.ib()
    tag = attr.ib()
    type = attr.ib(default='simple_text')


@attr.s
class CardWithButtons(ToDictMixin):
    text = attr.ib()
    buttons = attr.ib(default=attr.Factory(list))
    type = attr.ib(default='text_with_button')


@attr.s
class DivCard(ToDictMixin):
    body = attr.ib()
    type = attr.ib(default='div_card')
    text = attr.ib(default='...')  # TODO: remove it after client fix


@attr.s
class Div2Card(ToDictMixin):
    body = attr.ib()
    hide_borders = attr.ib()
    text = attr.ib()
    type = attr.ib(default='div2_card')


@attr.s
class ActionDirective(ToDictMixin):
    name = attr.ib()
    payload = attr.ib(default=attr.Factory(dict))
    type = attr.ib(default=None)

    @staticmethod
    def from_dict(data):
        if data['type'] == 'client_action':
            return ClientActionDirective(
                payload=data.get('payload'), name=data.get('name'), sub_name=data.get('sub_name')
            )
        elif data['type'] == 'uniproxy_action':
            return UniproxyActionDirective(payload=data.get('payload'), name=data.get('name'))
        elif data['type'] == 'server_action':
            return ServerActionDirective(
                payload=data.get('payload'), name=data.get('name'),
                ignore_answer=data.get('ignore_answer', False),
            )
        elif data['type'] == 'megamind_action':
            return MegamindActionDirective(payload=data.get('payload'), name=data.get('name'))
        else:
            raise ValueError("Wrong ActionDirective type '%s'", data.get('type'))


@attr.s
class ClientActionDirective(ActionDirective):
    sub_name = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'client_action'


@attr.s
class UniproxyActionDirective(ActionDirective):
    def __attrs_post_init__(self):
        self.type = 'uniproxy_action'


@attr.s
class ServerActionDirective(ActionDirective):
    ignore_answer = attr.ib(default=False)

    def __attrs_post_init__(self):
        self.type = 'server_action'


@attr.s
class MegamindActionDirective(ActionDirective):
    def __attrs_post_init__(self):
        self.type = 'megamind_action'


@attr.s
class TypedSemanticFrameDirective(ActionDirective):
    analytics = attr.ib(default={})

    def __attrs_post_init__(self):
        self.type = 'server_action'


@attr.s
class Button(ToDictMixin):
    title = attr.ib()
    type = attr.ib(default=None)

    @staticmethod
    def from_dict(data):
        directives = [ActionDirective.from_dict(d) for d in data['directives']]
        if data['type'] == 'action':
            return ActionButton(title=data['title'], type=data['type'], directives=directives)
        elif data['type'] == 'themed_action':
            return ThemedActionButton(title=data['title'], type=data['type'], theme=data['theme'], directives=directives)
        elif has_special_button(data['type']):
            return get_special_button(data['type'])(title=data['title'], type=data['type'], text=data['text'],
                                                    directives=directives)
        else:
            raise ValueError("Wrong Button type '%s'", data.get('type'))


@attr.s
class ActionButton(Button):
    directives = attr.ib(default=attr.Factory(list))

    def __attrs_post_init__(self):
        self.type = 'action'


@attr.s
class ThemedActionButton(ActionButton):
    theme = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'themed_action'


@attr.s
class SpecialButton(Button):
    directives = attr.ib(default=attr.Factory(list))
    text = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'special_button'


@attr.s
class LikeButton(SpecialButton):
    def __attrs_post_init__(self):
        self.type = 'like_button'


@attr.s
class DislikeButton(SpecialButton):
    def __attrs_post_init__(self):
        self.type = 'dislike_button'


@attr.s
class Meta(ToDictMixin):
    type = attr.ib(default=None)

    @classmethod
    def from_dict(cls, data):
        if has_meta(data.get('type')):
            return get_meta(data.get('type')).from_dict(data)
        else:
            raise ValueError("Wrong Meta type '%s'", data.get('type'))


@attr.s
class AnalyticsInfoMeta(Meta):
    intent = attr.ib(default=None)
    form = attr.ib(default=attr.Factory(dict))
    scenario_analytics_info_data = attr.ib(default=None)
    product_scenario_name = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'analytics_info'

    @staticmethod
    def parse_scenario_analytics_info(scenario_analytics_info_data):
        if scenario_analytics_info_data:
            message = TAnalyticsInfo()
            message.ParseFromString(base64.b64decode(scenario_analytics_info_data))
            return message
        return None

    def get_scenario_analytics_info(self):
        analytics_info = TAnalyticsInfo()
        if self.scenario_analytics_info_data:
            analytics_info.CopyFrom(self.scenario_analytics_info_data)
        if not analytics_info.Intent and self.intent:
            analytics_info.Intent = self.intent
        if not analytics_info.ProductScenarioName and self.product_scenario_name:
            analytics_info.ProductScenarioName = self.product_scenario_name
        return analytics_info

    def to_dict(self):
        result = super(AnalyticsInfoMeta, self).to_dict()

        # backward compatibility
        serialized_scenario_analytics_info = self.get_scenario_analytics_info().SerializeToString()
        result['scenario_analytics_info_data'] = base64.b64encode(serialized_scenario_analytics_info)

        return result

    @classmethod
    def from_dict(cls, data):
        scenario_analytics_info = cls.parse_scenario_analytics_info(data.get('scenario_analytics_info_data'))
        return cls(
            intent=data['intent'],
            form=data['form'],
            scenario_analytics_info_data=scenario_analytics_info,
        )


@attr.s
class FormRestoredMeta(Meta):
    overriden_form = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'form_restored'

    @classmethod
    def from_dict(cls, data):
        return cls(overriden_form=data['overriden_form'])


@attr.s
class ErrorMeta(Meta):
    error_type = attr.ib(default=None)
    form_name = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'error'

    @classmethod
    def from_dict(cls, data):
        if data['error_type'] == 'features_extractor_error':
            return FeaturesExtractorErrorMeta(features_extractor=data['features_extractor'])
        else:
            return cls(error_type=data['error_type'], form_name=data.get('form_name'))


@attr.s
class FeaturesExtractorErrorMeta(ErrorMeta):
    features_extractor = attr.ib(default=None)

    def __attrs_post_init__(self):
        super(FeaturesExtractorErrorMeta, self).__attrs_post_init__()
        self.error_type = 'features_extractor_error'


@attr.s
class ApplyArguments(ToDictMixin):
    # TODO(alkapov): to proto model
    form_update = attr.ib(default=None)
    callback = attr.ib(default=None)


@attr.s
class NlgRenderHistoryRecord(ToDictMixin):
    phrase_id = attr.ib(default=None)
    card_id = attr.ib(default=None)
    req_info = attr.ib(default=None)
    form = attr.ib(default=None)
    context = attr.ib(default=None)


@attr.s
class NlgRenderBassBlock(ToDictMixin):
    bass_block_dict = attr.ib(default=None)


@attr.s
class VinsResponse(ToDictMixin):
    """
    VinsResponse models aggregate VINS answer. At the moment it includes:
      voice_text - text that will be transformed into sound using tts engine
      cards - list of cards, currently two card types are supported:
        SimpleCard - simple text card
        CardWithButtons - card with several buttons
      directives - list of client and/or server actions. e.g. opening uri
      suggests - list of buttons
      special_buttons - list of special buttons
      should_listen - flag which tells client to keep listening (switch on microphone) after answer
      force_voice_answer - flag which tells that voice answer must be generated even if session is not voice
      sessions - new sessions state that would be persisted on uniproxy
      features -
      autoaction_delay_ms -
      apply_arguments - the arguments required for API calls with side effect application only
      commit_arguments - the arguments required for API calls separated into a pure part returning a response (run)
                         and a side effect that can potentially be executed asynchronously later (commit).
      continue_arguments - the arguments required for API calls without side effects to execute heavy requests
      commit_response - the raw protocol response got from BASS commit handle
      bass_response - the raw bass response got from BASS handle
      player_features - player features that the protocol response needs
      frame_actions - frame actions that the protocol response needs
      scenario_data - scenario data that the protocol response needs
      nlg_render_bass_blocks - bass blocks to render nlg in hollywood
      stack_engine - stack engine block
    """
    voice_text = attr.ib(default=None)
    cards = attr.ib(default=attr.Factory(list))
    directives = attr.ib(default=attr.Factory(list))
    suggests = attr.ib(default=attr.Factory(list))
    special_buttons = attr.ib(default=attr.Factory(list))
    should_listen = attr.ib(default=None)
    force_voice_answer = attr.ib(default=False)
    meta = attr.ib(default=attr.Factory(list))
    analytics_info_meta = attr.ib(default=None)
    sessions = attr.ib(default=None)
    features = attr.ib(default=attr.Factory(dict))
    autoaction_delay_ms = attr.ib(default=None)
    apply_arguments = attr.ib(default=None)
    commit_arguments = attr.ib(default=None)
    continue_arguments = attr.ib(default=None)
    commit_response = attr.ib(default=None)
    bass_response = attr.ib(default=None)
    player_features = attr.ib(default=None)
    frame_actions = attr.ib(default=attr.Factory(dict))
    templates = attr.ib(default=attr.Factory(dict))
    scenario_data = attr.ib(default=attr.Factory(dict))
    nlg_render_bass_blocks = attr.ib(default=attr.Factory(list))
    stack_engine = attr.ib(default=None)

    megamind_actions = attr.ib(default=attr.Factory(list))
    nlg_render_history_records = attr.ib(default=attr.Factory(list))

    def say(self, voice_text, card_text=None, append=False, buttons=None, should_listen=None, force_voice_answer=None,
            card_tag=None, autoaction_delay_ms=None):
        """
        This method allows to fill VinsResponse with information.

        :param voice_text: text that will be transformed into sound. If append flag is False previous value of
            voice_text will be overwritten. Otherwise the new value and the previous one will be concatenated using '\n'
            separator.
        :type voice_text: basestring
        :param card_text: text that will be displayed as a single text card. If append flag is False all previously
            created cards will be overwritten. Card will be created even if card_text is None. In this case voice_text
            will be used as card content. Note that either card_text or voice_text must not be None.
        :type card_text: basestring
        :param append: if set to True previously added information will be kept
        :type append: bool
        :param buttons:  if this parameter is not None card with buttons will be created instead of simple text card
        :type buttons: list of Button
        :param should_listen: flag which tells client to keep listening (switch on microphone) after answer.
            Specifying it here is a shortcut. You can always set it manually.
        :type should_listen: bool
        :type force_voice_answer: bool
        :param card_tag: if not None, tag field will be added to SimpleCard with value provided.
        :return: for convenience returns self
        :rtype: VinsResponse
        """

        if should_listen is not None:
            self.should_listen = should_listen

        if force_voice_answer is not None:
            self.force_voice_answer = force_voice_answer

        if card_text is None:
            if voice_text is None:
                raise RuntimeError('Either card_text or voice_text must not be None')

            card_text = voice_text

        if buttons:
            card = CardWithButtons(text=card_text, buttons=buttons)
        else:
            if card_text:
                card = SimpleCard(text=card_text, tag=card_tag)
            else:
                card = None

        if append:
            if self.voice_text:
                self.voice_text = '{0}\n{1}'.format(self.voice_text, voice_text) if voice_text else self.voice_text
            else:
                self.voice_text = voice_text

            if card:
                self.cards.append(card)
        else:
            self.voice_text = voice_text
            if card:
                self.cards = [card]

        if autoaction_delay_ms is not None:
            self.autoaction_delay_ms = autoaction_delay_ms

        return self

    def ask(self, *args, **kwargs):
        kwargs['should_listen'] = True
        return self.say(*args, **kwargs)

    def add_meta(self, meta):
        assert isinstance(meta, Meta)
        self.meta.append(meta)

    def get_meta(self):
        # backward compatibility
        result = [m.to_dict() for m in self.meta or []]
        if self.analytics_info_meta:
            result.append(self.analytics_info_meta.to_dict())
        return result

    def set_analytics_info(self, intent, form, scenario_analytics_info, product_scenario_name):
        analytics_info_meta = self.analytics_info_meta
        if not analytics_info_meta:
            analytics_info_meta = AnalyticsInfoMeta(
                intent=intent,
                form=form,
                scenario_analytics_info_data=scenario_analytics_info,
                product_scenario_name=product_scenario_name,
            )

        if intent:
            analytics_info_meta.intent = intent
        if form:
            analytics_info_meta.form = form
        if scenario_analytics_info:
            analytics_info_meta.scenario_analytics_info_data = scenario_analytics_info
        if product_scenario_name:
            analytics_info_meta.product_scenario_name = product_scenario_name

        self.analytics_info_meta = analytics_info_meta

    def add_megamind_action(self, action_frame, nlu_frame=None, nlu_hints=None):
        assert nlu_frame is not None or nlu_hints is not None

        megamind_action = TFrameAction()
        megamind_action.Frame.Name = action_frame
        if nlu_frame is not None:
            megamind_action.NluHint.FrameName = nlu_frame
        if nlu_hints is not None:
            lang_rus = 1
            for phrase in nlu_hints:
                instance = megamind_action.NluHint.Instances.add()
                instance.Phrase = phrase
                instance.Language = lang_rus

        self.megamind_actions.append((str(len(self.megamind_actions)), json.loads(MessageToJson(megamind_action))))

    def set_session(self, dialog_id, data):
        if not self.sessions:
            self.sessions = {}
        self.sessions[dialog_id if dialog_id else ''] = data

    def set_feature(self, feature):
        self.features[feature.FEATURE_TYPE] = feature

    def set_apply_arguments(self, apply_arguments):
        assert isinstance(apply_arguments, ApplyArguments)
        self.apply_arguments = apply_arguments

    def set_continue_arguments(self, continue_arguments):
        assert isinstance(continue_arguments, ApplyArguments)
        self.continue_arguments = continue_arguments

    def set_commit_arguments(self, commit_arguments):
        self.commit_arguments = commit_arguments

    def set_commit_response(self, commit_response):
        self.commit_response = commit_response

    def set_bass_response(self, bass_response):
        self.bass_response = bass_response

    def add_nlg_render_history_record(self, nlg_render_history_record):
        self.nlg_render_history_records.append(nlg_render_history_record)

    def add_nlg_render_bass_block(self, bass_block_dict):
        self.nlg_render_bass_blocks.append(NlgRenderBassBlock(bass_block_dict=bass_block_dict))

    @classmethod
    def from_dict(cls, data, strict=False):
        meta = _list_from_dict(data['meta'], Meta, strict)

        res = cls(
            voice_text=data.get('voice_text'),
            should_listen=data.get('should_listen'),
            force_voice_answer=data.get('force_voice_answer'),
            cards=_list_from_dict(data['cards'], Card, strict),
            directives=_list_from_dict(data['directives'], ActionDirective, strict),
            suggests=_list_from_dict(data['suggests'], Button, strict),
            special_buttons=_list_from_dict(data.get('special_buttons'), SpecialButton, strict),
            meta=list(filter(lambda item: not isinstance(item, AnalyticsInfoMeta), meta)),
            analytics_info_meta=next((item for item in meta if isinstance(item, AnalyticsInfoMeta)), None),
            features=_features_from_dict(data.get('features'), strict),
            autoaction_delay_ms=data.get('autoaction_delay_ms'),
            apply_arguments=data.get('apply_arguments'),
            continue_arguments=data.get('continue_arguments'),
            player_features=data.get('player_features'),
            nlg_render_history_records=_list_from_dict(data.get('nlg_render_history_records'), NlgRenderHistoryRecord, strict),
            nlg_render_bass_blocks=_list_from_dict(data.get('nlg_render_bass_blocks'), NlgRenderBassBlock, strict)
        )
        return res

    def to_dict(self):
        result = super(VinsResponse, self).to_dict()

        # backward compatibility
        analytics_info_meta = result.pop('analytics_info_meta')
        if analytics_info_meta:
            if result.get('meta'):
                result['meta'].append(analytics_info_meta)
            else:
                result['meta'] = [analytics_info_meta]

        del result['sessions']
        del result['apply_arguments']
        del result['continue_arguments']
        del result['commit_arguments']
        del result['commit_response']
        del result['bass_response']
        del result['megamind_actions']
        del result['nlg_render_history_records']
        del result['nlg_render_bass_blocks']
        del result['player_features']
        return result


def _list_from_dict(lst, cls, strict):
    result = []
    if not lst:
        return result
    for item in lst:
        try:
            result.append(cls.from_dict(item))
        except Exception:
            logger.exception('Response deserialization error on object %s', item)
            if strict:
                raise
    return result


def _features_from_dict(d, strict):
    result = {}
    if not d:
        return result
    for type_, item in d.iteritems():
        try:
            result[type_] = _get_feature_cls(type_).from_dict(item)
        except Exception:
            logger.exception('Response deserialization error on object %s', item)
            if strict:
                raise
    return result


@attr.s
class FormInfoFeatures(ToDictMixin):
    FEATURE_TYPE = 'form_info'

    is_continuing = attr.ib(default=False)
    is_irrelevant = attr.ib(default=False)
    intent = attr.ib(default=None)
    expects_request = attr.ib(default=False)
    answers_expected_request = attr.ib(default=False)

    @classmethod
    def from_dict(cls, data):
        return cls(
            is_continuing=data['is_continuing'],
            intent=data.get('intent'),
            is_irrelevant=data.get('is_irrelevant'),
            expects_request=data.get('expects_request'),
            answers_expected_request=data.get('answers_expected_request'),
        )


register_meta(AnalyticsInfoMeta, 'analytics_info')
register_meta(FormRestoredMeta, 'form_restored')
register_meta(ErrorMeta, 'error')

register_special_button(SpecialButton, 'special_button')
register_special_button(LikeButton, 'like_button')
register_special_button(DislikeButton, 'dislike_button')

register_feature(FormInfoFeatures)
