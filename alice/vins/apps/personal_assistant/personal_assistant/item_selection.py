# coding: utf-8
from __future__ import unicode_literals

import logging
import math
import attr
import json
import re

from collections import defaultdict, OrderedDict

from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.utils.lemmer import Lemmer
from vins_core.utils.data import load_data_from_file
from vins_core.utils.strings import get_translit

from personal_assistant import intents
from personal_assistant import clients
from personal_assistant.view_state.lib.extract_active_video_items import extract_active_video_items

logger = logging.getLogger(__name__)


class ItemMatcherBase(object):
    def get_score(self, text1, text2):
        # Should return matching score
        raise NotImplementedError


class TfIdfItemMatcher(ItemMatcherBase):
    _lemmer = Lemmer(['ru', 'en'])
    _use_lemmer_parse_top = 2

    def __init__(self, number_idf=0.25, prcl_conj_prep_idf=0.1, helper_idf=0.25, helper_words=None):
        self._number_idf = number_idf
        self._prcl_conj_prep_idf = prcl_conj_prep_idf
        self._helper_idf = helper_idf
        self._helper_words = set(helper_words or {})

    def _get_idf(self, parse):
        # Ignore particles and prepositions in matching
        if parse.tag.POS in ['PRCL', 'CONJ', 'PREP', 'INTJ']:
            return self._prcl_conj_prep_idf

        # Numbers contribute less
        if parse.word.isdigit():
            return self._number_idf

        # Helper words (defined by the subject area, e.g. tv) contribute less
        if parse.word.lower() in self._helper_words:
            return self._helper_idf

        return 1.0

    def _text_to_tokens(self, text):
        # Do not do any heavy normalization using FSTs, assuming it has been done before if needed
        text = text.replace('-', ' ')
        text = text.lower()
        return text.split()

    def _text_to_weighted_parse(self, text):
        tokens = self._text_to_tokens(text)
        weighted_parse = []
        for token in tokens:
            normal_forms = set()
            parses = []
            for parse in self._lemmer.parse(token)[:self._use_lemmer_parse_top]:
                if parse.normal_form not in normal_forms and self._get_idf(parse) > 0.0:
                    normal_forms.add(parse.normal_form)
                    parses.append(parse)

            # If there is more than one parse for a token, reduce the relative contribution of each
            for parse in parses:
                weighted_parse.append((1.0 / len(parses), parse))

        return weighted_parse

    def _to_vector(self, text):
        result = defaultdict(float)
        weighted_parse = self._text_to_weighted_parse(text)
        for weight, parse in weighted_parse:
            vector_component = self._get_idf(parse) * weight
            if vector_component > 0.0:
                # Add both word and normal form, so that the exact match is preferred
                result['word__' + parse.word] += vector_component
                result['normal_form__' + parse.normal_form] += vector_component

        return result

    def _dot(self, vec1, vec2):
        result = 0.0
        for k, v1 in vec1.iteritems():
            v2 = vec2.get(k, 0.0)
            result += v1 * v2
        return result

    def _len(self, vec):
        len_sqr = 0.0
        for v in vec.itervalues():
            len_sqr += v * v
        return math.sqrt(len_sqr)

    def _cos(self, vec1, vec2):
        denominator = self._len(vec1) * self._len(vec2)
        return self._dot(vec1, vec2) / denominator if denominator > 0 else 0.0

    def get_score(self, text1, text2):
        vec1 = self._to_vector(text1)
        vec2 = self._to_vector(text2)
        return self._cos(vec1, vec2)


@attr.s
class Item(object):
    texts = attr.ib()
    data = attr.ib()


def get_current_screen(req_info):
    if not req_info:
        return None

    device_state = req_info.device_state
    if not device_state or 'video' not in device_state:
        return None

    video_state = device_state.get('video')
    if not video_state or 'current_screen' not in video_state:
        return None
    return video_state.get('current_screen')


class ItemSelectorBase(object):
    """
    By default selectors that have nothing to select from are restricted
    by transition model (by setting the corresponding intent weight to 0.0).
    In some cases, however, this could be undesireable (for example, when an
    intent has multiple callbacks, and not just `on_item_selection`), hence
    `allows_empty_item_list` is used to override this behavior. See `call__ellipsis`
    for an example.
    """
    allows_empty_item_list = False

    def __init__(self, matcher=None):
        self._matcher = matcher or TfIdfItemMatcher()

    @property
    def matcher(self):
        return self._matcher

    def get_items(self, req_info, form):
        """
        Returns a list of items to select from.
        """
        raise NotImplementedError

    def get_score_threshold(self):
        """
        Returns a score threshold
        """
        return 0.47

    def get_text_to_match(self, form, sample, req_info):
        """
        Returns the text to match against the items.
        """
        raise NotImplementedError

    def can_handle_intent(self, intent_name, req_info):
        """
        Returns a value indicating whether the item selector should work in the context of the given intent.
        """
        raise NotImplementedError

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        """
        Called when an item has been successfully selected.
        """
        raise NotImplementedError


class EtherVideoItemSelector(ItemSelectorBase):
    _allowed_screens = {'mordovia_webview'}

    def __init__(self, extractor=None, synonym_files=None, cache_size=30000, cache_file=None, **kwargs):
        super(EtherVideoItemSelector, self).__init__(**kwargs)
        if not extractor:
            self._extractor = SamplesExtractor(pipeline=[NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)])
        else:
            self._extractor = extractor

    def get_score_threshold(self):
        return 0.6

    def get_items(self, req_info, form):
        # turn off vins-based ether item selector if Hollywood version enabled
        if req_info.experiments['mm_enable_protocol_scenario=MordoviaVideoSelection'] is not None:
            return None
        device_state = req_info.device_state or {}
        video_state = device_state.get('video') or {}
        current_screen = video_state.get('current_screen')
        if current_screen not in self._allowed_screens or clients.is_smart_speaker_without_screen(req_info):
            return None

        view_state = video_state.get('view_state') or video_state.get('page_state') or {}
        video_items = extract_active_video_items(view_state)

        result = []
        for video_item in video_items:
            result.append(
                Item(
                    texts=self.get_item_texts(video_item),
                    data={
                        'url': video_item['metaforback'].get('url'),
                        'uuid': video_item['metaforback'].get('uuid'),
                        'streams': video_item['metaforback'].get('streams'),
                        'description': video_item['metaforback'].get('description'),
                        'duration': video_item['metaforback'].get('duration'),
                        'thumbnail': video_item['metaforback'].get('thumbnail'),
                        'title': video_item['metaforback'].get('title'),
                        'serial_id': video_item['metaforback'].get('serial_id'),
                        'subscriptions': video_item['metaforback'].get('subscriptions'),
                        'index': None
                    }
                )
            )

        return result

    def get_text_to_match(self, form, sample, req_info):
        return form.video_text.value

    def can_handle_intent(self, intent_name, req_info):
        current_screen = get_current_screen(req_info)

        return intents.is_quasar_ether_gallery_selection_by_text(
            intent_name) and current_screen and current_screen in self._allowed_screens

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        new_form = app.new_form(intents.ETHER_QUASAR_VIDEO_SELECT, req_info)
        new_form.content_url.set_value(item.data['url'], 'string')
        new_form.uuid.set_value(item.data['uuid'], 'string')
        new_form.streams.set_value(json.dumps(item.data['streams']), 'string')
        if item.data['description']:
            new_form.description.set_value(item.data['description'], 'string')
        if item.data['duration']:
            new_form.duration.set_value(item.data['duration'], 'number')
        if item.data['thumbnail']:
            new_form.thumbnail.set_value(item.data['thumbnail'], 'string')
        if item.data['title']:
            new_form.title.set_value(item.data['title'], 'string')
        if item.data['serial_id']:
            new_form.serial_id.set_value(item.data['serial_id'], 'string')
        if item.data['subscriptions']:
            new_form.subscriptions.set_value(','.join(item.data['subscriptions']), 'string')
        new_form.action.set_value(form.action.value, form.action.value_type)

        app.change_form(session, new_form, req_info, sample, response)

    def normalize_item_text(self, text):
        normalized = self._extractor([text])[0].text

        return normalized

    def get_item_texts(self, item):
        number = item['number']
        result = [
            '%d' % number,
            'номер %d' % number
        ]
        result.append(self.normalize_item_text(item['title']))

        return result


class VideoGalleryItemSelector(ItemSelectorBase):
    _allowed_screens = {'gallery', 'season_gallery'}

    def __init__(self, extractor=None, synonym_files=None, cache_size=30000, cache_file=None, **kwargs):
        super(VideoGalleryItemSelector, self).__init__(**kwargs)
        if not extractor:
            self._extractor = SamplesExtractor(pipeline=[NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)])
        else:
            self._extractor = extractor
        self._normalization_cache = OrderedDict()
        if cache_file is not None:
            cache_init = load_data_from_file(cache_file)
            for initial_text, values in cache_init.iteritems():
                for transliterate, value in values.iteritems():
                    self._normalization_cache[(initial_text, False if transliterate == 'false' else True)] = value
        self._cache_size = cache_size
        self.phrase_to_id = {}
        self.id_to_phrase = defaultdict(set)
        if synonym_files is not None:
            for filename in synonym_files:
                vocabulary = load_data_from_file(filename)
                for key, texts in vocabulary.iteritems():
                    for text in texts:
                        normalized = self.normalize_item_text(text, transliterate=True, cache=True)
                        old_key = self.phrase_to_id.get(normalized)
                        if old_key is not None and old_key != key:
                            logger.debug('{} collide with {} on text {}'.format(old_key, key, normalized))
                            for text_to_move in self.id_to_phrase[old_key]:
                                self.phrase_to_id[text_to_move] = key
                                self.id_to_phrase[key].add(text_to_move)
                            del self.id_to_phrase[old_key]
                        self.phrase_to_id[normalized] = key
                        self.id_to_phrase[key].add(normalized)

    def get_synonyms(self, text):
        return self.id_to_phrase.get(self.phrase_to_id.get(text), {text})

    def get_items(self, req_info, form):
        device_state = req_info.device_state or {}
        video_state = device_state.get('video') or {}
        current_screen = video_state.get('current_screen')
        if current_screen not in self._allowed_screens:
            return None

        screen_state = video_state.get('screen_state') or {}
        gallery_items = screen_state.get('items') or []
        visible_item_indices = screen_state.get('visible_items') or []

        if req_info.experiments['quasar_video_dont_use_visible_items'] is not None:
            visible_item_indices = xrange(len(gallery_items))
            logger.debug('Visible indices overriden to %s', visible_item_indices)

        visible_item_indices = set(visible_item_indices)

        result = []
        for index, gallery_item in enumerate(gallery_items):
            result.append(
                Item(
                    texts=self.get_item_texts(
                        gallery_item,
                        index,
                        is_visible=(index in visible_item_indices),
                        current_screen=current_screen
                    ),
                    data={
                        'item': gallery_item,
                        'index': index
                    }
                )
            )
        return result

    def get_text_to_match(self, form, sample, req_info):
        return form.video_text.value

    def can_handle_intent(self, intent_name, req_info):
        current_screen = get_current_screen(req_info)

        return intents.is_quasar_video_gallery_selection_by_text(
            intent_name) and current_screen and current_screen in self._allowed_screens

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        new_form = app.new_form(intents.QUASAR_SELECT_VIDEO_FROM_GALLERY, req_info)
        new_form.video_index.set_value(item.data['index'] + 1, 'num')
        new_form.action.set_value(form.action.value, form.action.value_type)
        new_form.content_provider.set_value(form.content_provider.value, form.content_provider.value_type)
        app.change_form(session, new_form, req_info, sample, response)

    def normalize_item_text(self, text, transliterate=False, cache=False):
        if cache:
            cached = self._normalization_cache.pop((text, transliterate), None)
            if cached is not None:
                self._normalization_cache[(text, transliterate)] = cached
                return cached
        normalized = self._extractor([text])[0].text
        if transliterate:
            normalized = get_translit(normalized)
        if cache:
            self._normalization_cache[(text, transliterate)] = normalized
            if len(self._normalization_cache) > self._cache_size:
                self._normalization_cache.popitem(last=False)
        return normalized

    def get_item_texts(self, item, index, is_visible, current_screen=None):
        result = [
            '%d' % (index + 1),
            'номер %d' % (index + 1)
        ]
        if is_visible and isinstance(item.get('name'), basestring):
            result.append(self.normalize_item_text(item['name']))
        return result


class TVGalleryItemSelector(VideoGalleryItemSelector):
    _allowed_screens = {'tv_gallery'}

    def can_handle_intent(self, intent_name, req_info):
        return intents.is_quasar_channel_gallery_selection_by_text(intent_name)

    def get_text_to_match(self, form, sample, req_info):
        return self.normalize_item_text(form.video_text.value, transliterate=True)

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        super(TVGalleryItemSelector, self).on_item_selected(item, app, session, form, req_info, sample, response, score)
        if score < 0.75:
            response.say('Я не уверена, но, кажется, вы имели в виду {}.'.format(item.data['item']['name']))

    def get_item_texts(self, item, index, is_visible, current_screen=None):
        result = [
            '%d' % (index + 1),
            'номер %d' % (index + 1)
        ]
        # we enable only visible films and tv programs, but all tv channels
        result.extend(list(self.get_synonyms(self.normalize_item_text(item['name'], transliterate=True, cache=True))))
        tv_episode_name = item.get('tv_episode_name', '')
        if tv_episode_name:
            result.append(self.normalize_item_text(tv_episode_name, transliterate=True, cache=True))
        # look at the entity dict for current channel name
        result.append(self.normalize_item_text(item['name'], transliterate=True, cache=True))
        return result


class ContactItemSelector(ItemSelectorBase):
    allows_empty_item_list = True

    # Mapping from Android values to VINS entities
    # NOTE: other phone types are technically available in contacts
    # provider, but appear too exotic to include them here
    phone_types = {
        'TYPE_WORK': 'work',
        'TYPE_HOME': 'home',
        'TYPE_MOBILE': 'mobile'
    }
    item_type_phone = 'phone'
    item_type_contact = 'contact'

    def get_items(self, req_info, form):
        if not form or not form.get_slot_by_name('contact_search_results'):
            return []
        contacts = form.contact_search_results.value or []
        if not contacts or not self._check_contacts_preprocessed(contacts):
            return []

        # we could search either contacts or specific phone numbers
        if self._is_searching_phones(form):
            items = contacts[0]['phones']
            item_type = self.item_type_phone
            text_extractor = self._extract_phone_type
        else:
            items = contacts
            item_type = self.item_type_contact
            text_extractor = self._extract_contact_name

        return [
            Item(
                texts=[
                    '%d' % (index + 1),
                    'номер %d' % (index + 1),
                    text_extractor(items[index])
                ],
                data={
                    'item': items[index],
                    'index': index,
                    'type': item_type
                }
            )
            for index in xrange(len(items))
        ]

    def get_text_to_match(self, form, sample, req_info):
        if self._is_searching_phones(form):
            return self._get_phone(form)
        else:
            return self._get_contact_name(form)

    def can_handle_intent(self, intent_name, req_info):
        return intents.is_call_ellipsis(intent_name) or intents.is_messaging_ellipsis(intent_name)

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        if item.data['type'] == self.item_type_contact:
            result = [item.data['item']]
        else:
            # if searching for phones, there must be at most one contact
            assert self._is_searching_phones(form)
            result = form.contact_search_results.value
            result[0]['phones'] = [item.data['item']]

        form.contact_search_results.set_value(
            result, 'contact_search_results'
        )

    @classmethod
    def _check_contacts_preprocessed(cls, contacts):
        # before we're able to select anything, contacts
        # should be preprocessed by BASS and grouped by name

        # TODO: should we make it more explicit by setting a flag somewhere?
        return not contacts or 'phones' in contacts[0]

    @classmethod
    def _extract_contact_name(cls, item):
        return item['name']

    @classmethod
    def _extract_phone_type(cls, item):
        return cls.phone_types.get(item['phone_type_name'], '')

    def _is_searching_phones(self, form):
        contacts = form.contact_search_results.value or []
        return self._check_contacts_preprocessed(contacts) and len(contacts) == 1

    def _get_contact_name(self, form):
        if form.recipient.value_type == 'fio':
            return u' '.join({
                form.recipient.value[component]
                for component in ('surname', 'name', 'patronym')
                if component in form.recipient.value
            })
        elif form.recipient.value_type == 'num':
            return str(form.recipient.value)
        else:
            return form.recipient.value

    def _get_phone(self, form):
        # NOTE: some form ambiguity here: <number> can mean either
        # contact or phone, dependent on currently displayed list of contacts.
        # so if `recipient` is a number and `_is_searching_phones` condition is
        # True, treat it as a phone selector.
        if form.phone_type.value is not None:
            return form.phone_type.value
        if form.recipient.value_type == 'num':
            return str(form.recipient.value)

        assert False, 'Can not extract phone description from slots'


class ExactTaxiCardMatch(ItemMatcherBase):
    def get_score(self, text1, text2):
        if text1 == text2:
            return 1
        elif text1 == "" or text2 == "":  # fake element: item selector must select something
            return 0.9
        else:
            return 0


class SelectTaxiCardNumber(ItemSelectorBase):
    allows_empty_item_list = False

    def __init__(self):
        self._matcher = ExactTaxiCardMatch()

    @property
    def matcher(self):
        return self._matcher

    def get_items(self, req_info, form):
        if not form or not form.get_slot_by_name('allowed_cards'):
            return []
        cards = form.get_slot_by_name('allowed_cards').value or []
        card_items = [
            Item([card.get('card_number'), str(num + 1)], card)
            for num, card in enumerate(cards)
        ] if isinstance(cards, list) else []

        # fake element: item selector must select something because we need visit bass
        card_items.append(Item([""], {}))
        return card_items

    def get_text_to_match(self, form, sample, req_info):
        return ''.join(re.findall(r'\d+', sample.text))

    def can_handle_intent(self, intent_name, req_info):
        return intent_name == 'personal_assistant.scenarios.taxi_new_order__select_card'

    def on_item_selected(self, item, app, session, form, req_info, sample, response, score=None):
        if not form or not form.get_slot_by_name('selected_card'):
            return
        form.selected_card.set_value(item.data, 'string')
        app.universal_callback(req_info=req_info, session=session, response=response, sample=sample, form=form)


_registered_item_selectors = []


def register_item_selector(item_selector):
    _registered_item_selectors.append(item_selector)


def get_item_selector_for_intent(intent_name, req_info):
    intent_item_selectors = [
        its for its in _registered_item_selectors if its.can_handle_intent(intent_name, req_info)
    ]
    error_text = 'Only one item selector is currently supported per intent, have {} for intent {}'.format(
        intent_item_selectors,
        intent_name
    )
    assert len(intent_item_selectors) <= 1, error_text
    return next(iter(intent_item_selectors), None)
