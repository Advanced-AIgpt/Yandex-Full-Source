# coding: utf-8
from __future__ import unicode_literals

import attr
import logging
import urllib

from vins_core.utils.config import get_setting

from personal_assistant import clients

from autoapp_directives_translator import autoapp_allowed_intents

logger = logging.getLogger('intents')

# Intent name prefixes
APP_INTENT_PREFIX = 'personal_assistant.'
STROKA_INTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'stroka.'
SCENARIOS_NAME_PREFIX = APP_INTENT_PREFIX + 'scenarios.'
MARKET_SCENARIOS_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'market'
MARKET_BERU_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'market_beru'
MARKET_NATIVE_BERU_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'market_native_beru'
MARKET_CHECKOUT_PREFIX = SCENARIOS_NAME_PREFIX + 'market__checkout'
RECURRING_PURCHASE_PREFIX = SCENARIOS_NAME_PREFIX + 'recurring_purchase'
SHOPPING_LIST_PREFIX = SCENARIOS_NAME_PREFIX + 'shopping_list'
SHOPPING_LIST_FIXLIST_SUFFIX = '_fixlist'
QUASAR_SCENARIOS_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'quasar.'
MICROINTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'handcrafted.'
AVIA_SCENARIOS_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'avia'
SHOW_COLLECTION_SCENARIOS_NAME_PREFIX = SCENARIOS_NAME_PREFIX + 'show_collection'
QUASAR_MICROINTENTS_PREFIX = MICROINTENT_NAME_PREFIX + 'quasar.'
DRIVE_MICROINTENTS_PREFIX = MICROINTENT_NAME_PREFIX + 'drive.'
# TODO(autoapp): remove when autoapp's dead
AUTOAPP_MICROINTENTS_PREFIX = MICROINTENT_NAME_PREFIX + 'autoapp.'
FEEDBACK_NEGATIVE_PREFIX = APP_INTENT_PREFIX + 'feedback.feedback_negative'
FEEDBACK_POSITIVE_PREFIX = APP_INTENT_PREFIX + 'feedback.feedback_positive'
FIND_POI_PREFIX = SCENARIOS_NAME_PREFIX + 'find_poi'
GC_FEEDBACK_PREFIX = APP_INTENT_PREFIX + 'feedback.gc_feedback_'
GC_FEEDBACK_POSITIVE_PREFIX = APP_INTENT_PREFIX + 'feedback.gc_feedback_positive'
GC_FEEDBACK_NEUTRAL_PREFIX = APP_INTENT_PREFIX + 'feedback.gc_feedback_neutral'
GC_FEEDBACK_NEGATIVE_PREFIX = APP_INTENT_PREFIX + 'feedback.gc_feedback_negative'
GENERAL_CONVERSATION_PREFIX = APP_INTENT_PREFIX + 'general_conversation.'
INTERNAL_PREFIX = APP_INTENT_PREFIX + 'internal.'
EXTERNAL_SKILL_PREFIX = SCENARIOS_NAME_PREFIX + 'external_skill'
RADIO_PREFIX = SCENARIOS_NAME_PREFIX + 'radio_play'
DIRECT_GALLERY_PREFIX = SCENARIOS_NAME_PREFIX + 'direct_gallery'
SEARCH_PREFIX = SCENARIOS_NAME_PREFIX + 'search'
SHOW_ROUTE_PREFIX = SCENARIOS_NAME_PREFIX + 'show_route'
TV_PREFIX = SCENARIOS_NAME_PREFIX + 'tv'
OPEN_SITE_OR_APP_PREFIX = SCENARIOS_NAME_PREFIX + 'open_site_or_app'
AUTOMOTIVE_INTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'automotive.'
AUTOMOTIVE_GREETING = AUTOMOTIVE_INTENT_NAME_PREFIX + 'greeting'
NAVIGATOR_INTENT_NAME_PREFIX = APP_INTENT_PREFIX + 'navi.'
MESSAGING_PREFIX = SCENARIOS_NAME_PREFIX + 'messaging'

# Other useful constants
INTERNAL_INTENT_SEPARATOR = '__'
ELLIPSIS_INTENT_SUFFIX = INTERNAL_INTENT_SEPARATOR + 'ellipsis'

# Intent names
SESSION_START = INTERNAL_PREFIX + 'session_start'
HARDCODED = INTERNAL_PREFIX + 'hardcoded'
FEEDBACK_POSITIVE = FEEDBACK_POSITIVE_PREFIX
FEEDBACK_NEGATIVE = FEEDBACK_NEGATIVE_PREFIX
FEEDBACK_NEGATIVE_IMAGES = FEEDBACK_NEGATIVE_PREFIX + '_images'
GC_FEEDBACK_POSITIVE = GC_FEEDBACK_POSITIVE_PREFIX
GC_FEEDBACK_NEUTRAL = GC_FEEDBACK_NEUTRAL_PREFIX
GC_FEEDBACK_NEGATIVE = GC_FEEDBACK_NEGATIVE_PREFIX
GENERAL_CONVERSATION_DUMMY = GENERAL_CONVERSATION_PREFIX + 'general_conversation_dummy'
GENERAL_CONVERSATION = GENERAL_CONVERSATION_PREFIX + 'general_conversation'
PURE_GC = SCENARIOS_NAME_PREFIX + 'pure_general_conversation'
PURE_GC_EXIT = SCENARIOS_NAME_PREFIX + 'pure_general_conversation_deactivation'
LET_US_TALK_MICROINTENT = MICROINTENT_NAME_PREFIX + 'let_us_talk'
EXTERNAL_SKILL_DEACTIVATE = EXTERNAL_SKILL_PREFIX + INTERNAL_INTENT_SEPARATOR + 'deactivate'
EXTERNAL_SKILL_ACTIVATE = EXTERNAL_SKILL_PREFIX
ONBOARDING = SCENARIOS_NAME_PREFIX + 'onboarding'
CALL_ELLIPSIS = SCENARIOS_NAME_PREFIX + 'call' + ELLIPSIS_INTENT_SUFFIX
MESSAGING_ELLIPSIS_PREFIX = SCENARIOS_NAME_PREFIX + 'messaging' + INTERNAL_INTENT_SEPARATOR
ONBOARDING_NEXT = ONBOARDING + '__next'
ONBOARDING_CANCEL = ONBOARDING + '__cancel'
QUASAR_SELECT_VIDEO_FROM_GALLERY = QUASAR_SCENARIOS_NAME_PREFIX + 'select_video_from_gallery'
QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT = QUASAR_SCENARIOS_NAME_PREFIX + 'select_video_from_gallery_by_text'
QUASAR_SELECT_ETHER_FROM_GALLERY_BY_TEXT = QUASAR_SCENARIOS_NAME_PREFIX + 'select_ether_from_gallery_by_text'
QUASAR_SELECT_CHANNEL_FROM_GALLERY_BY_TEXT = QUASAR_SCENARIOS_NAME_PREFIX + 'select_channel_from_gallery_by_text'
QUASAR_PAYMENT_CONFIRMED = QUASAR_SCENARIOS_NAME_PREFIX + 'payment_confirmed'
QUASAR_AUTHORIZE_VIDEO_PROVIDER = QUASAR_SCENARIOS_NAME_PREFIX + 'authorize_video_provider'
QUASAR_OPEN_CURRENT_VIDEO = QUASAR_SCENARIOS_NAME_PREFIX + 'open_current_video'
QUASAR_GO_TO_VIDEO_SCREEN = QUASAR_SCENARIOS_NAME_PREFIX + 'goto_video_screen'
QUASAR_GO_FORWARD = QUASAR_SCENARIOS_NAME_PREFIX + 'go_forward'
QUASAR_GO_BACKWARD = QUASAR_SCENARIOS_NAME_PREFIX + 'go_backward'
QUASAR_GO_UP = QUASAR_SCENARIOS_NAME_PREFIX + 'go_up'
QUASAR_GO_DOWN = QUASAR_SCENARIOS_NAME_PREFIX + 'go_down'
QUASAR_GO_TOP = QUASAR_SCENARIOS_NAME_PREFIX + 'go_top'
QUASAR_GO_TO_THE_BEGINNING = QUASAR_SCENARIOS_NAME_PREFIX + 'go_to_the_beginning'
QUASAR_GO_TO_THE_END = QUASAR_SCENARIOS_NAME_PREFIX + 'go_to_the_end'
QUASAR_GO_HOME = QUASAR_SCENARIOS_NAME_PREFIX + 'go_home'

ETHER_QUASAR_VIDEO_SELECT = SCENARIOS_NAME_PREFIX + 'ether.quasar.video_select'

IMAGE_RECOGNIZER_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this'
MUSIC_RECOGNIZER_INTENT = SCENARIOS_NAME_PREFIX + 'music_what_is_playing'
PLAYER_CONTINUE_INTENT = SCENARIOS_NAME_PREFIX + 'player_continue'
OPEN_OR_CONTINUE_INTENT = SCENARIOS_NAME_PREFIX + 'quasar.open_or_continue'
PLAYER_NEXT_INTENT = SCENARIOS_NAME_PREFIX + 'player_next_track'
PLAYER_PREVIOUS_INTENT = SCENARIOS_NAME_PREFIX + 'player_previous_track'

LIST_CANCEL_INTENT = SCENARIOS_NAME_PREFIX + 'common.cancel_list'
ALARM_STOP_PLAYING_INTENT = SCENARIOS_NAME_PREFIX + 'alarm_stop_playing'
TIMER_STOP_PLAYING_INTENT = SCENARIOS_NAME_PREFIX + 'timer_stop_playing'

ALARM_SNOOZE_INTENT_ABS = SCENARIOS_NAME_PREFIX + 'alarm_snooze_abs'
ALARM_SNOOZE_INTENT_REL = SCENARIOS_NAME_PREFIX + 'alarm_snooze_rel'

ALARM_SOUND_SET_LEVEL_INTENT = SCENARIOS_NAME_PREFIX + 'alarm_sound_set_level'

IMAGE_WHAT_IS_THIS_OCR_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this_ocr'
IMAGE_WHAT_IS_THIS_MARKET_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this_market'
IMAGE_WHAT_IS_THIS_CLOTHES_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this_clothes'
IMAGE_WHAT_IS_THIS_SIMILAR_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this_similar'
IMAGE_WHAT_IS_THIS_BARCODE_INTENT = SCENARIOS_NAME_PREFIX + 'image_what_is_this_barcode'

HAPPY_NEW_YEAR_INTENT = SCENARIOS_NAME_PREFIX + 'hny.'
TEACH_ME_INTENT = SCENARIOS_NAME_PREFIX + 'teach_me'
HELLO_MICROINTENT = MICROINTENT_NAME_PREFIX + 'hello'
DEAF_MICROINTENT = MICROINTENT_NAME_PREFIX + 'deaf'

BROWSER_READ_PAGE_PAUSE_INTENT = SCENARIOS_NAME_PREFIX + 'browser_read_page_pause'
BROWSER_READ_PAGE_CONTINUE_INTENT = SCENARIOS_NAME_PREFIX + 'browser_read_page_continue'
PROHIBITION_ERROR_INTENT = SCENARIOS_NAME_PREFIX + 'common.prohibition_error'
SHAZAM_INTENTS = [
    SCENARIOS_NAME_PREFIX + 'music_what_is_playing',
    SCENARIOS_NAME_PREFIX + 'music_what_is_playing__ellipsis'
]

# Irrelevant intents
IRRELEVANT_ERROR_INTENT_PREFIX = SCENARIOS_NAME_PREFIX + 'common.irrelevant'
IRRELEVANT_ERROR_INTENT_DEFAULT = IRRELEVANT_ERROR_INTENT_PREFIX
IRRELEVANT_ERROR_INTENTS = {
    EXTERNAL_SKILL_ACTIVATE: IRRELEVANT_ERROR_INTENT_PREFIX + '.external_skill'
}

# Multi-step scenario. If the parent intent represents a multi-step scenario,
# then some of the intents will be marked as 'restricted exit': in this case, we provide a
# list of intents that we are allowed to transition into.
# The below structure is a map{from_intent, (can_leave_scenario, list<allowed_to_intent>)}
MULTISTEP_SCENARIO_TRANSITIONS = {
    # Voiceprint enroll
    SCENARIOS_NAME_PREFIX + "voiceprint_enroll": (False, [
        SCENARIOS_NAME_PREFIX + "voiceprint_enroll__collect_voice",
        SCENARIOS_NAME_PREFIX + "voiceprint_enroll__finish"
    ]),
    SCENARIOS_NAME_PREFIX + "voiceprint_enroll__collect_voice": (False, [
        SCENARIOS_NAME_PREFIX + "voiceprint_enroll__collect_voice",
        SCENARIOS_NAME_PREFIX + "voiceprint_enroll__finish"
    ]),
    SCENARIOS_NAME_PREFIX + "voiceprint_enroll__finish": (True, [
        SCENARIOS_NAME_PREFIX + "voiceprint_enroll"
    ]),
    # Bugreport
    INTERNAL_PREFIX + "bugreport": (True, [
        INTERNAL_PREFIX + "bugreport",
        INTERNAL_PREFIX + "bugreport__continue",
        INTERNAL_PREFIX + "bugreport__deactivate"
    ]),
    INTERNAL_PREFIX + "bugreport__continue": (False, [
        INTERNAL_PREFIX + "bugreport__continue",
        INTERNAL_PREFIX + "bugreport__deactivate"
    ]),
    INTERNAL_PREFIX + "bugreport__deactivate": (True, [
        INTERNAL_PREFIX + "bugreport"
    ]),
    # Voiceprint remove
    SCENARIOS_NAME_PREFIX + "voiceprint_remove": (False, [
        SCENARIOS_NAME_PREFIX + "voiceprint_remove__confirm",
        SCENARIOS_NAME_PREFIX + "voiceprint_remove__finish"
    ]),
    SCENARIOS_NAME_PREFIX + "voiceprint_remove__confirm": (False, [
        SCENARIOS_NAME_PREFIX + "voiceprint_remove__confirm"
    ]),
    SCENARIOS_NAME_PREFIX + "voiceprint_remove__finish": (True, [
        SCENARIOS_NAME_PREFIX + "voiceprint_remove"
    ]),
}


def get_market_state(form):
    if not form:
        return None
    form_slot = form.get_slot_by_name('market__state')
    if not form_slot:
        return None
    return form_slot.value


STATEFUL_SCENARIOS = {
    RECURRING_PURCHASE_PREFIX: get_market_state,
    RECURRING_PURCHASE_PREFIX + '__login': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__cancel': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__garbage': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__repeat': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__ellipsis': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__number_filter': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__product_details': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_delivery_intervals': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_items_number': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_index': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_yes_or_no': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_suits': get_market_state,
    RECURRING_PURCHASE_PREFIX + '__checkout_everything': get_market_state,
    MESSAGING_PREFIX: MESSAGING_PREFIX,
    MESSAGING_PREFIX + '__recipient': MESSAGING_PREFIX + '__recipient',
    MESSAGING_PREFIX + '__client': MESSAGING_PREFIX + '__client',
    MESSAGING_PREFIX + '__text': MESSAGING_PREFIX + '__text',
    MESSAGING_PREFIX + '__recipient_ellipsis': MESSAGING_PREFIX + '__recipient_ellipsis',
    MARKET_SCENARIOS_NAME_PREFIX: get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__market': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__market__ellipsis': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__start_choice_again': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__number_filter': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__garbage': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__show_more': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__repeat': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__product_details': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__add_to_cart': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__cancel': get_market_state,
    MARKET_SCENARIOS_NAME_PREFIX + '__go_to_shop': get_market_state,
    MARKET_CHECKOUT_PREFIX: get_market_state,
    MARKET_CHECKOUT_PREFIX + "_items_number": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_email": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_phone": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_address": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_index": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_delivery_intervals": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_yes_or_no": get_market_state,
    MARKET_CHECKOUT_PREFIX + "_everything": get_market_state,
}

BASE_RECURRING_PURCHASE_INTENTS = [
    RECURRING_PURCHASE_PREFIX,
    RECURRING_PURCHASE_PREFIX + '__cancel',
    RECURRING_PURCHASE_PREFIX + '__garbage',
    RECURRING_PURCHASE_PREFIX + '__repeat',
    RECURRING_PURCHASE_PREFIX + '__ellipsis',
    RECURRING_PURCHASE_PREFIX + '__number_filter',
    RECURRING_PURCHASE_PREFIX + '__checkout_everything',
]

BASE_MARKET_CHOICE_INTENTS = [
    MARKET_SCENARIOS_NAME_PREFIX + "__market",
    MARKET_SCENARIOS_NAME_PREFIX + "__market__ellipsis",
    MARKET_SCENARIOS_NAME_PREFIX + "__number_filter",
    MARKET_SCENARIOS_NAME_PREFIX + "__garbage",
    MARKET_SCENARIOS_NAME_PREFIX + "__start_choice_again",
    MARKET_SCENARIOS_NAME_PREFIX + "__cancel",
    MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
]

BASE_MARKET_CHOICE_OPEN_INTENTS = [
    MARKET_SCENARIOS_NAME_PREFIX + "__market",
    MARKET_SCENARIOS_NAME_PREFIX + "__market__ellipsis",
    MARKET_SCENARIOS_NAME_PREFIX + "__number_filter",
    MARKET_SCENARIOS_NAME_PREFIX + "__start_choice_again",
    MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
]

BASE_MARKET_CHECKOUT_INTENTS = [
    MARKET_CHECKOUT_PREFIX + "_everything",
    MARKET_SCENARIOS_NAME_PREFIX + "__cancel",
    MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
    MARKET_SCENARIOS_NAME_PREFIX + "__start_choice_again",
]

STATEFUL_SCENARIO_TRANSITIONS = {
    'Market.RecurringPurchase.RecurringPurchase': (False, BASE_RECURRING_PURCHASE_INTENTS),
    'Market.RecurringPurchase.SelectProductIndex': (False, [
        RECURRING_PURCHASE_PREFIX,
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_index',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.Choice': (False, BASE_RECURRING_PURCHASE_INTENTS),
    'Market.RecurringPurchase.Login': (True, [
        RECURRING_PURCHASE_PREFIX,
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__login',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything'
    ]),
    'Market.RecurringPurchase.ProductDetailsScreenless': (False, [
        RECURRING_PURCHASE_PREFIX,
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_yes_or_no',
        RECURRING_PURCHASE_PREFIX + '__checkout',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.ProductDetails': (False, BASE_RECURRING_PURCHASE_INTENTS + [
        RECURRING_PURCHASE_PREFIX + '__checkout'
    ]),
    'Market.RecurringPurchase.CheckoutItemsNumber': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
        RECURRING_PURCHASE_PREFIX + '__checkout_items_number',
    ]),
    'Market.RecurringPurchase.CheckoutPhone': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.CheckoutAddress': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.CheckoutDeliveryOptions': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_index',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
        RECURRING_PURCHASE_PREFIX + '__checkout_delivery_intervals',
    ]),
    'Market.RecurringPurchase.CheckoutDeliveryFirstOption': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_suits',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.CheckoutConfirmOrder': (False, [
        RECURRING_PURCHASE_PREFIX + '__cancel',
        RECURRING_PURCHASE_PREFIX + '__repeat',
        RECURRING_PURCHASE_PREFIX + '__checkout_yes_or_no',
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.CheckoutWaiting': (False, [
        RECURRING_PURCHASE_PREFIX + '__checkout_everything',
    ]),
    'Market.RecurringPurchase.CheckoutComplete': (True, [
        RECURRING_PURCHASE_PREFIX,
        RECURRING_PURCHASE_PREFIX + '__ellipsis',
        RECURRING_PURCHASE_PREFIX + '__repeat',
    ]),
    'Market.RecurringPurchase.Exit': (True, [RECURRING_PURCHASE_PREFIX]),
    MESSAGING_PREFIX: (False, [
        MESSAGING_PREFIX + '__recipient',
        MESSAGING_PREFIX + '__client',
        MESSAGING_PREFIX + '__text',
        MESSAGING_PREFIX + '__recipient_ellipsis',
    ]),
    MESSAGING_PREFIX + '__recipient': (False, [
        MESSAGING_PREFIX + '__client',
        MESSAGING_PREFIX + '__text',
        MESSAGING_PREFIX + '__recipient_ellipsis',
    ]),
    MESSAGING_PREFIX + '__recipient_ellipsis': (False, [
        MESSAGING_PREFIX + '__client',
        MESSAGING_PREFIX + '__text',
    ]),
    MESSAGING_PREFIX + '__client': (False, [
        MESSAGING_PREFIX + '__text',
    ]),
    MESSAGING_PREFIX + '__text': (True, []),
    'Market.Choice.Activation': (False, [
        MARKET_SCENARIOS_NAME_PREFIX + "__market",
        MARKET_SCENARIOS_NAME_PREFIX + "__market__ellipsis",
        MARKET_SCENARIOS_NAME_PREFIX + "__garbage",
        MARKET_SCENARIOS_NAME_PREFIX + "__cancel",
        MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
    ]),
    'Market.Choice.Choice': (False, BASE_MARKET_CHOICE_INTENTS),
    'Market.Choice.ProductDetails': (False, BASE_MARKET_CHOICE_INTENTS + [
        MARKET_CHECKOUT_PREFIX,
        MARKET_SCENARIOS_NAME_PREFIX + "__beru_order",
        MARKET_SCENARIOS_NAME_PREFIX + "__go_to_shop",
    ]),
    'Market.Choice.BeruProductDetails': (False, BASE_MARKET_CHOICE_INTENTS + [
        MARKET_SCENARIOS_NAME_PREFIX + "__go_to_shop",
        MARKET_CHECKOUT_PREFIX,
    ]),
    'Market.Choice.MakeOrder': (False, BASE_MARKET_CHOICE_INTENTS + [
        MARKET_CHECKOUT_PREFIX,
    ]),
    'Market.Choice.Activation.Open': (True, [
        MARKET_SCENARIOS_NAME_PREFIX + "__market",
        MARKET_SCENARIOS_NAME_PREFIX + "__market__ellipsis",
        MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
    ]),
    'Market.Choice.Choice.Open': (True, BASE_MARKET_CHOICE_OPEN_INTENTS),
    'Market.Choice.ProductDetails.Open': (True, BASE_MARKET_CHOICE_OPEN_INTENTS + [
        MARKET_CHECKOUT_PREFIX,
        MARKET_SCENARIOS_NAME_PREFIX + "__beru_order",
        MARKET_SCENARIOS_NAME_PREFIX + "__go_to_shop",
    ]),
    'Market.Choice.BeruProductDetails.Open': (True, BASE_MARKET_CHOICE_OPEN_INTENTS + [
        MARKET_SCENARIOS_NAME_PREFIX + "__go_to_shop",
        MARKET_CHECKOUT_PREFIX,
    ]),
    'Market.Choice.MakeOrder.Open': (False, BASE_MARKET_CHOICE_OPEN_INTENTS + [
        MARKET_CHECKOUT_PREFIX,
    ]),
    'Market.Choice.CheckoutItemsNumber': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_items_number",
    ]),
    'Market.Choice.CheckoutEmail': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_email",
    ]),
    'Market.Choice.CheckoutPhone': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_phone",
    ]),
    'Market.Choice.CheckoutAddress': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_address",
        MARKET_CHECKOUT_PREFIX + "_yes_or_no",
        MARKET_CHECKOUT_PREFIX + "_index",
    ]),
    'Market.Choice.CheckoutDeliveryOptions': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_index",
        MARKET_CHECKOUT_PREFIX + "_delivery_intervals",
    ]),
    'Market.Choice.CheckoutConfirmOrder': (False, BASE_MARKET_CHECKOUT_INTENTS + [
        MARKET_CHECKOUT_PREFIX + "_yes_or_no",
    ]),
    'Market.Choice.CheckoutWaiting': (False, BASE_MARKET_CHECKOUT_INTENTS),
    'Market.Choice.CheckoutComplete': (True, [
        MARKET_SCENARIOS_NAME_PREFIX + "__market",
        MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
        MARKET_SCENARIOS_NAME_PREFIX + "__start_choice_again",
    ]),
    'Market.Choice.Exit': (True, [
        MARKET_SCENARIOS_NAME_PREFIX + "__repeat",
    ]),
}

NAVI_SAFE_SCENARIOS = {
    PROHIBITION_ERROR_INTENT,
    GENERAL_CONVERSATION_PREFIX + 'general_conversation',
    GENERAL_CONVERSATION_PREFIX + 'general_conversation_dummy',
    MICROINTENT_NAME_PREFIX,
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point',
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point__cancel',
    NAVIGATOR_INTENT_NAME_PREFIX + 'add_point__ellipsis',
    NAVIGATOR_INTENT_NAME_PREFIX + 'change_voice',
    NAVIGATOR_INTENT_NAME_PREFIX + 'change_voice__ellipsis',
    NAVIGATOR_INTENT_NAME_PREFIX + 'hide_layer',
    NAVIGATOR_INTENT_NAME_PREFIX + 'how_long_to_drive',
    NAVIGATOR_INTENT_NAME_PREFIX + 'how_long_traffic_jam',
    NAVIGATOR_INTENT_NAME_PREFIX + 'onboarding',
    NAVIGATOR_INTENT_NAME_PREFIX + 'parking_route',
    NAVIGATOR_INTENT_NAME_PREFIX + 'reset_route',
    NAVIGATOR_INTENT_NAME_PREFIX + 'show_layer',
    NAVIGATOR_INTENT_NAME_PREFIX + 'show_route_on_map',
    NAVIGATOR_INTENT_NAME_PREFIX + 'when_we_get_there',
    SCENARIOS_NAME_PREFIX + 'convert',
    SCENARIOS_NAME_PREFIX + 'convert__ellipsis',
    SCENARIOS_NAME_PREFIX + 'convert__get_info',
    SCENARIOS_NAME_PREFIX + 'external_skill',
    SCENARIOS_NAME_PREFIX + 'external_skill__activate_only',
    SCENARIOS_NAME_PREFIX + 'external_skill__continue',
    SCENARIOS_NAME_PREFIX + 'external_skill__deactivate',
    SCENARIOS_NAME_PREFIX + 'find_poi',
    SCENARIOS_NAME_PREFIX + 'find_poi__details',
    SCENARIOS_NAME_PREFIX + 'find_poi__ellipsis',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__by_index',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__next',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__prev',
    SCENARIOS_NAME_PREFIX + 'find_poi__show_on_map',
    SCENARIOS_NAME_PREFIX + 'games_onboarding',
    SCENARIOS_NAME_PREFIX + 'get_date',
    SCENARIOS_NAME_PREFIX + 'get_date__ellipsis',
    SCENARIOS_NAME_PREFIX + 'get_my_location',
    SCENARIOS_NAME_PREFIX + 'get_my_location__details',
    SCENARIOS_NAME_PREFIX + 'get_news',
    SCENARIOS_NAME_PREFIX + 'get_news__ellipsis',
    SCENARIOS_NAME_PREFIX + 'get_news__more',
    SCENARIOS_NAME_PREFIX + 'get_time',
    SCENARIOS_NAME_PREFIX + 'get_time__ellipsis',
    SCENARIOS_NAME_PREFIX + 'get_weather',
    SCENARIOS_NAME_PREFIX + 'get_weather__ellipsis',
    SCENARIOS_NAME_PREFIX + 'music_sing_song',
    SCENARIOS_NAME_PREFIX + 'music_sing_song__next',
    SCENARIOS_NAME_PREFIX + 'random_num',
    SCENARIOS_NAME_PREFIX + 'remember_named_location',
    SCENARIOS_NAME_PREFIX + 'remember_named_location__ellipsis',
    SCENARIOS_NAME_PREFIX + 'repeat',
    SCENARIOS_NAME_PREFIX + 'search__show_on_map',
    SCENARIOS_NAME_PREFIX + 'set_my_name',
    SCENARIOS_NAME_PREFIX + 'show_route',
    SCENARIOS_NAME_PREFIX + 'show_route__ellipsis',
    SCENARIOS_NAME_PREFIX + 'show_route__show_route_on_map',
    SCENARIOS_NAME_PREFIX + 'show_route__show_route_on_map_spec',
    SCENARIOS_NAME_PREFIX + 'show_route__taxi',
    SCENARIOS_NAME_PREFIX + 'show_traffic',
    SCENARIOS_NAME_PREFIX + 'show_traffic__details',
    SCENARIOS_NAME_PREFIX + 'sound_mute',
    SCENARIOS_NAME_PREFIX + 'sound_unmute',
    SCENARIOS_NAME_PREFIX + 'what_can_you_do',
    SCENARIOS_NAME_PREFIX + 'what_is_my_name',
    SHOPPING_LIST_PREFIX + '_add',
    SHOPPING_LIST_PREFIX + '_show',
    SHOPPING_LIST_PREFIX + '_show__show',
    SHOPPING_LIST_PREFIX + '_show__add',
    SHOPPING_LIST_PREFIX + '_show__delete_all',
    SHOPPING_LIST_PREFIX + '_show__delete_index',
    SHOPPING_LIST_PREFIX + '_show__delete_item',
    SHOPPING_LIST_PREFIX + '_delete_item',
    SHOPPING_LIST_PREFIX + '_delete_all',
    SHOPPING_LIST_PREFIX + '_login',
    PURE_GC,
    PURE_GC_EXIT,
    PURE_GC + '_what_can_you_do',
    PURE_GC + '_phone_call'
}
# TODO(autoapp): remove below const and 2 methods when autoapp's dead
OLD_AUTO_SAFE_SCENARIOS = autoapp_allowed_intents()


def is_autoapp_microintent(intent_name):
    return intent_name.startswith(AUTOAPP_MICROINTENTS_PREFIX)


def is_safe_for_autoapp(intent_name):
    return intent_name in OLD_AUTO_SAFE_SCENARIOS or is_autoapp_microintent(intent_name)


def is_music_safe_for_navigator_safe_mode(req_info):
    return clients.has_alicesdk_player(req_info)


def is_safe_for_navigator_safe_mode(intent_name, req_info):
    is_music_or_player_intent = is_music_intent(intent_name) or is_player_intent(intent_name)
    return (
        is_music_or_player_intent and is_music_safe_for_navigator_safe_mode(req_info) or
        intent_name in NAVI_SAFE_SCENARIOS or
        intent_name.startswith(MICROINTENT_NAME_PREFIX)
    )


ELARI_SAFE_SCENARIOS = {
    PROHIBITION_ERROR_INTENT,
    GENERAL_CONVERSATION_PREFIX + 'general_conversation',
    GENERAL_CONVERSATION_DUMMY,
    GC_FEEDBACK_POSITIVE,
    GC_FEEDBACK_NEUTRAL,
    GC_FEEDBACK_NEGATIVE,
    FEEDBACK_POSITIVE,
    FEEDBACK_NEGATIVE,
    SESSION_START,
    HARDCODED,
    ONBOARDING,
    ONBOARDING_NEXT,
    ONBOARDING_CANCEL,
    SCENARIOS_NAME_PREFIX + 'alarm_cancel',
    SCENARIOS_NAME_PREFIX + 'alarm_cancel__ellipsis',
    SCENARIOS_NAME_PREFIX + 'alarm_set',
    SCENARIOS_NAME_PREFIX + 'alarm_set__ellipsis',
    SCENARIOS_NAME_PREFIX + 'alarm_set',
    SCENARIOS_NAME_PREFIX + 'alarm_show',
    SCENARIOS_NAME_PREFIX + 'alarm_show__cacnel',
    SCENARIOS_NAME_PREFIX + 'alarm_snooze_abs',
    SCENARIOS_NAME_PREFIX + 'alarm_snooze_rel',
    SCENARIOS_NAME_PREFIX + 'alarm_stop_playing',
    SCENARIOS_NAME_PREFIX + 'bluetooth_on',
    SCENARIOS_NAME_PREFIX + 'bluetooth_off',
    SCENARIOS_NAME_PREFIX + 'call',
    SCENARIOS_NAME_PREFIX + 'call__ellipsis',
    SCENARIOS_NAME_PREFIX + 'common.cancel_list',
    SCENARIOS_NAME_PREFIX + 'convert',
    SCENARIOS_NAME_PREFIX + 'convert__ask',
    SCENARIOS_NAME_PREFIX + 'convert__ellipsis',
    SCENARIOS_NAME_PREFIX + 'convert__get_info',
    EXTERNAL_SKILL_ACTIVATE,
    EXTERNAL_SKILL_PREFIX + INTERNAL_INTENT_SEPARATOR + 'continue',
    EXTERNAL_SKILL_DEACTIVATE,
    SCENARIOS_NAME_PREFIX + 'find_poi',
    SCENARIOS_NAME_PREFIX + 'find_poi__ellipsis',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__by_index',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__next',
    SCENARIOS_NAME_PREFIX + 'find_poi__scroll__prev',
    SCENARIOS_NAME_PREFIX + 'find_poi__details',
    SCENARIOS_NAME_PREFIX + 'games_onboarding',
    SCENARIOS_NAME_PREFIX + 'get_date',
    SCENARIOS_NAME_PREFIX + 'get_date__ellipsis',
    SCENARIOS_NAME_PREFIX + 'get_my_location',
    SCENARIOS_NAME_PREFIX + 'get_time',
    SCENARIOS_NAME_PREFIX + 'get_time__ellipsis',
    SCENARIOS_NAME_PREFIX + 'get_weather',
    SCENARIOS_NAME_PREFIX + 'get_weather__ellipsis',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_similar_people',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_frontal',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_frontal_similar_people',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_ocr_voice',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_market',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_clothes',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_translate',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_barcode',
    SCENARIOS_NAME_PREFIX + 'image_what_is_this_similar_artwork',
    SCENARIOS_NAME_PREFIX + 'how_much',
    SCENARIOS_NAME_PREFIX + 'onboarding',
    SCENARIOS_NAME_PREFIX + 'onboarding__cancel',
    SCENARIOS_NAME_PREFIX + 'onboarding__next',
    SCENARIOS_NAME_PREFIX + 'random_num',
    SCENARIOS_NAME_PREFIX + 'random_num__ellipsis',
    SCENARIOS_NAME_PREFIX + 'remember_named_location',
    SCENARIOS_NAME_PREFIX + 'remember_named_location__ellipsis',
    SCENARIOS_NAME_PREFIX + 'alarm_reminder',
    SCENARIOS_NAME_PREFIX + 'create_reminder',
    SCENARIOS_NAME_PREFIX + 'create_reminder__cancel',
    SCENARIOS_NAME_PREFIX + 'create_reminder__ellipsis',
    SCENARIOS_NAME_PREFIX + 'list_reminders',
    SCENARIOS_NAME_PREFIX + 'list_reminders__scroll_next',
    SCENARIOS_NAME_PREFIX + 'list_reminders__scroll_stop',
    SCENARIOS_NAME_PREFIX + 'repeat',
    SCENARIOS_NAME_PREFIX + 'search',
    SCENARIOS_NAME_PREFIX + 'search_anaphoric',
    SCENARIOS_NAME_PREFIX + 'show_route',
    SCENARIOS_NAME_PREFIX + 'show_route__ellipsis',
    SCENARIOS_NAME_PREFIX + 'show_traffic',
    SCENARIOS_NAME_PREFIX + 'show_traffic__ellipsis',
    SCENARIOS_NAME_PREFIX + 'timer_cancel',
    SCENARIOS_NAME_PREFIX + 'timer_cancel__ellipsis',
    SCENARIOS_NAME_PREFIX + 'timer_pause',
    SCENARIOS_NAME_PREFIX + 'timer_pause__ellipsis',
    SCENARIOS_NAME_PREFIX + 'timer_resume',
    SCENARIOS_NAME_PREFIX + 'timer_resume__ellipsis',
    SCENARIOS_NAME_PREFIX + 'timer_set',
    SCENARIOS_NAME_PREFIX + 'timer_set__ellipsis',
    SCENARIOS_NAME_PREFIX + 'timer_show',
    SCENARIOS_NAME_PREFIX + 'timer_show__cancel',
    SCENARIOS_NAME_PREFIX + 'timer_show__pause',
    SCENARIOS_NAME_PREFIX + 'timer_show__resume',
    SCENARIOS_NAME_PREFIX + 'timer_stop_playing',
    SCENARIOS_NAME_PREFIX + 'translate',
    SCENARIOS_NAME_PREFIX + 'translate__ellipsis',
    SCENARIOS_NAME_PREFIX + 'translate__quicker',
    SCENARIOS_NAME_PREFIX + 'translate__slower',
    SCENARIOS_NAME_PREFIX + 'create_todo',
    SCENARIOS_NAME_PREFIX + 'create_todo__cancel',
    SCENARIOS_NAME_PREFIX + 'create_todo__ellipsis',
    SCENARIOS_NAME_PREFIX + 'list_todo',
    SCENARIOS_NAME_PREFIX + 'list_todo__scroll_next',
    SCENARIOS_NAME_PREFIX + 'list_todo__scroll_stop',
    SCENARIOS_NAME_PREFIX + 'repeat_after_me',
    SCENARIOS_NAME_PREFIX + 'get_weather_nowcast',
    SCENARIOS_NAME_PREFIX + 'get_weather_nowcast__ellipsis',
    PURE_GC,
    PURE_GC_EXIT,
    PURE_GC + '_what_can_you_do',
    PURE_GC + '_phone_call',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order',
    SCENARIOS_NAME_PREFIX + 'taxi_new_disabled',
}

HAPPY_NEW_YEAR_BLOGGERS = {
    HAPPY_NEW_YEAR_INTENT + 'varlamov',
    HAPPY_NEW_YEAR_INTENT + 'suhov',
    HAPPY_NEW_YEAR_INTENT + 'trubenkova',
    HAPPY_NEW_YEAR_INTENT + 'may',
    HAPPY_NEW_YEAR_INTENT + 'bubenitta',
    HAPPY_NEW_YEAR_INTENT + 'wylsacom',
    HAPPY_NEW_YEAR_INTENT + 'smetana',
    HAPPY_NEW_YEAR_INTENT + 'slivki',
    HAPPY_NEW_YEAR_INTENT + 'semenihin',
    HAPPY_NEW_YEAR_INTENT + 'pokashevarim',
    HAPPY_NEW_YEAR_INTENT + 'anohina',
    HAPPY_NEW_YEAR_INTENT + 'lisovec',
    HAPPY_NEW_YEAR_INTENT + 'viskunova',
    HAPPY_NEW_YEAR_INTENT + 'jarahov',
    HAPPY_NEW_YEAR_INTENT + 'gagarina',
    HAPPY_NEW_YEAR_INTENT + 'dakota',
    HAPPY_NEW_YEAR_INTENT + 'review',
    HAPPY_NEW_YEAR_INTENT + 'bekmambetov',
    HAPPY_NEW_YEAR_INTENT + 'parfenon',
    HAPPY_NEW_YEAR_INTENT + 'grilkov',
    HAPPY_NEW_YEAR_INTENT + 'badcomedian'
}


def is_safe_for_elari_watch(intent_name):
    return intent_name in ELARI_SAFE_SCENARIOS or intent_name.startswith(MICROINTENT_NAME_PREFIX)


VINS_IRRELEVANT_INTENTS = {
    'personal_assistant.handcrafted.goto_blogger_external_skill',
    'personal_assistant.scenarios.external_skill',
    'personal_assistant.scenarios.external_skill__activate_only',
    'personal_assistant.scenarios.market_orders_status',
    'personal_assistant.scenarios.market_orders_status__login',
    'personal_assistant.scenarios.music_play_less',
    'personal_assistant.scenarios.music_play_more',
    'personal_assistant.scenarios.image_what_is_this',
    'personal_assistant.scenarios.image_what_is_this_barcode',
    'personal_assistant.scenarios.image_what_is_this_clothes',
    'personal_assistant.scenarios.image_what_is_this_frontal',
    'personal_assistant.scenarios.image_what_is_this_frontal_similar_people',
    'personal_assistant.scenarios.image_what_is_this_market',
    'personal_assistant.scenarios.image_what_is_this_ocr',
    'personal_assistant.scenarios.image_what_is_this_ocr_voice',
    'personal_assistant.scenarios.image_what_is_this_office_lens',
    'personal_assistant.scenarios.image_what_is_this_similar_artwork',
    'personal_assistant.scenarios.image_what_is_this_similar_people',
    'personal_assistant.scenarios.image_what_is_this_similar',
    'personal_assistant.scenarios.image_what_is_this_translate',
    'personal_assistant.scenarios.image_what_is_this__clothes',
    'personal_assistant.scenarios.image_what_is_this__clothes_forced',
    'personal_assistant.scenarios.image_what_is_this__details',
    'personal_assistant.scenarios.image_what_is_this__ocr',
    'personal_assistant.scenarios.image_what_is_this__market',
    'personal_assistant.scenarios.image_what_is_this__similar',
    'personal_assistant.scenarios.image_what_is_this__info',
    'personal_assistant.scenarios.image_what_is_this__similar_people',
    'personal_assistant.scenarios.image_what_is_this__similar_artwork',
    'personal_assistant.scenarios.image_what_is_this__ocr_voice',
    'personal_assistant.scenarios.image_what_is_this__ocr_voice_suggest',
    'personal_assistant.scenarios.image_what_is_this__office_lens_disk',
    'personal_assistant.scenarios.get_news',
    'personal_assistant.scenarios.get_news__ellipsis',
    'personal_assistant.scenarios.get_news__details',
    'personal_assistant.scenarios.get_news__more',
    'personal_assistant.scenarios.get_weather',
    'personal_assistant.scenarios.get_weather__ellipsis',
    'personal_assistant.scenarios.get_weather_nowcast',
    'personal_assistant.scenarios.get_weather_nowcast__ellipsis',
    'personal_assistant.scenarios.get_weather__details',
    'personal_assistant.scenarios.translate',
    'personal_assistant.scenarios.translate__ellipsis',
    'personal_assistant.scenarios.translate__quicker',
    'personal_assistant.scenarios.translate__slower',
    'personal_assistant.scenarios.call',
}

VINS_IRRELEVANT_INTENTS_SOUND_COMMANDS = {
    'personal_assistant.scenarios.sound_louder',
    'personal_assistant.scenarios.sound_quiter',
    'personal_assistant.scenarios.sound_set_level',
    'personal_assistant.scenarios.sound_get_level',
    'personal_assistant.scenarios.sound_mute',
    'personal_assistant.scenarios.sound_unmute'
}

VINS_IRRELEVANT_INTENTS_PAUSE_COMMANDS = {
    'personal_assistant.handcrafted.cancel',
    'personal_assistant.handcrafted.fast_cancel',
    'personal_assistant.scenarios.player_pause'
}

VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS = {
    ONBOARDING
}

VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS_AND_TV_DEVICES = {
    'personal_assistant.scenarios.music_podcast',
    'personal_assistant.scenarios.video_play',
    'personal_assistant.scenarios.video_play_entity',
    'personal_assistant.scenarios.quasar.goto_video_screen',
    'personal_assistant.scenarios.quasar.payment_confirmed',
    'personal_assistant.scenarios.quasar.authorize_video_provider'
}

SCENARIO_ALWAYS_RELEVANT_INTENTS = {
    'personal_assistant.feedback.feedback_negative',
    'personal_assistant.feedback.feedback_positive',
}


@attr.s()
class ScenarioRelevantIntents(object):
    default_product_scenario_name = attr.ib()
    intents = attr.ib(default=attr.Factory(set))


@attr.s()
class SkipRelevantIntents(object):
    experiment = attr.ib(default=attr.Factory(str))
    # tuples of tuples for skip scenarios if all intents exists
    list_intents = attr.ib(default=attr.Factory(tuple))


# Key - Scenario name in path url
# Value:
#   * intents: Set with relevant intents
#   * default_product_scenario_name: product_scenario_name for irrelevant situation
SCENARIO_RELEVANT_INTENTS = {
    'bluetooth': ScenarioRelevantIntents(default_product_scenario_name='bluetooth', intents={
        'personal_assistant.scenarios.bluetooth_on',
        'personal_assistant.scenarios.bluetooth_off',
    }),
    'show_traffic': ScenarioRelevantIntents(default_product_scenario_name='show_traffic', intents={
        'personal_assistant.scenarios.show_traffic',
        'personal_assistant.scenarios.show_traffic__details',
        'personal_assistant.scenarios.show_traffic__ellipsis',
    }),
    'weather': ScenarioRelevantIntents(default_product_scenario_name='weather', intents={
        'personal_assistant.scenarios.get_weather',
        'personal_assistant.scenarios.get_weather__details',
        'personal_assistant.scenarios.get_weather__ellipsis',
        'personal_assistant.scenarios.get_weather_nowcast',
        'personal_assistant.scenarios.get_weather_nowcast__ellipsis',
    }),
    'alarm': ScenarioRelevantIntents(default_product_scenario_name='alarm', intents={
        'personal_assistant.scenarios.get_ask_sound',
        'personal_assistant.scenarios.get_ask_time',
        'personal_assistant.scenarios.alarm_cancel',
        'personal_assistant.scenarios.alarm_cancel__ellipsis',
        'personal_assistant.scenarios.alarm_how_long',
        'personal_assistant.scenarios.alarm_how_to_set_sound',
        'personal_assistant.scenarios.alarm_reset_sound_set',
        'personal_assistant.scenarios.alarm_set',
        'personal_assistant.scenarios.alarm_set_sound',
        'personal_assistant.scenarios.alarm_set_with_sound',
        'personal_assistant.scenarios.alarm_show__cancel',
        'personal_assistant.scenarios.alarm_show',
        'personal_assistant.scenarios.alarm_snooze_abs',
        'personal_assistant.scenarios.alarm_snooze_rel',
        'personal_assistant.scenarios.alarm_set_level',
        'personal_assistant.scenarios.alarm_stop_playing',
        'personal_assistant.scenarios.alarm_what_sound_is_set',
        'personal_assistant.scenarios.alarm_what_sound_level_is_set'
    }),
    'route': ScenarioRelevantIntents(default_product_scenario_name='route', intents={
        SHOW_ROUTE_PREFIX,
        SHOW_ROUTE_PREFIX + '__ellipsis',
        SHOW_ROUTE_PREFIX + '__show_route_on_map',
        SCENARIOS_NAME_PREFIX + 'remember_named_location',
        SCENARIOS_NAME_PREFIX + 'remember_named_location__ellipsis',
        SCENARIOS_NAME_PREFIX + 'taxi_new_call_to_support',
        SCENARIOS_NAME_PREFIX + 'taxi_new_cancel__confirmation_yes',
        SCENARIOS_NAME_PREFIX + 'taxi_new_cancel__confirmation_no',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_yes',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_no',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_wrong',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order__change_payment_or_tariff',
        SCENARIOS_NAME_PREFIX + 'taxi_new_order__specify',
        SCENARIOS_NAME_PREFIX + 'taxi_new_show_driver_info',
        SCENARIOS_NAME_PREFIX + 'taxi_new_show_legal',
        SCENARIOS_NAME_PREFIX + 'taxi_new_status',
        SCENARIOS_NAME_PREFIX + 'taxi_new_disabled',
        SCENARIOS_NAME_PREFIX + 'taxi_order',
    }),
}


def is_session_start(intent_name):
    return intent_name == SESSION_START


def parent_intent_name(intent_name):
    internal_separator_pos = intent_name.find(INTERNAL_INTENT_SEPARATOR)
    if internal_separator_pos != -1:
        return intent_name[:internal_separator_pos]
    else:
        return intent_name


def is_ellipsis(intent_name):
    return intent_name is not None and intent_name.endswith(ELLIPSIS_INTENT_SUFFIX)


def is_debug_intent(intent_name):
    return intent_name == INTERNAL_PREFIX + 'bugreport'


def is_skill(intent_name):
    return intent_name is not None and intent_name.startswith(EXTERNAL_SKILL_PREFIX)


def is_skill_deactivate(intent_name):
    return intent_name == EXTERNAL_SKILL_DEACTIVATE


def is_skill_continue(intent_name):
    return intent_name == EXTERNAL_SKILL_PREFIX + INTERNAL_INTENT_SEPARATOR + 'continue'


def is_skill_activate(intent_name):
    return intent_name == EXTERNAL_SKILL_ACTIVATE


def is_skill_activate_only(intent_name):
    return intent_name == EXTERNAL_SKILL_PREFIX + INTERNAL_INTENT_SEPARATOR + 'activate_only'


def is_multistep_scenario(intent_name):
    return intent_name in MULTISTEP_SCENARIO_TRANSITIONS


def is_disallowed_multistep_transition(prev_intent_name, intent_name):
    if prev_intent_name not in MULTISTEP_SCENARIO_TRANSITIONS:
        return False

    can_leave, next_steps = MULTISTEP_SCENARIO_TRANSITIONS[prev_intent_name]

    if intent_name in next_steps:
        # This is an explicitly allowed transition
        return False

    if not can_leave:
        # This is not an allowed transition, and external transitions are disabled
        return True

    if parent_intent_name(intent_name) == parent_intent_name(prev_intent_name):
        # We can leave, but aren't leaving
        return True

    # Ok, leaving is allowed
    return False


def is_stateful_scenario(intent_name):
    return intent_name in STATEFUL_SCENARIOS


def get_state(form, intent_name):
    if not is_stateful_scenario(intent_name):
        return None

    state_func = STATEFUL_SCENARIOS[intent_name]

    if callable(state_func):
        return state_func(form)
    else:
        return state_func


def is_open_stateful_transition(prev_form, prev_intent_name):
    if not is_stateful_scenario(prev_intent_name):
        return False

    state = get_state(prev_form, prev_intent_name)

    if state is None:
        return True

    if state not in STATEFUL_SCENARIO_TRANSITIONS:
        return True

    can_leave, _ = STATEFUL_SCENARIO_TRANSITIONS[state]

    return can_leave


def is_disallowed_stateful_transition(prev_form, prev_intent_name, intent_name):
    if not is_stateful_scenario(prev_intent_name):
        return False

    state = get_state(prev_form, prev_intent_name)

    if state is None:
        logger.info('State is None (%s->%s)', prev_intent_name, intent_name)
        return is_stateful_scenario(intent_name) and parent_intent_name(intent_name) != intent_name

    if state not in STATEFUL_SCENARIO_TRANSITIONS:
        logger.error('Unknown state %s in (%s->%s)', state, prev_intent_name, intent_name)
        return parent_intent_name(intent_name) == parent_intent_name(prev_intent_name)

    can_leave, next_steps = STATEFUL_SCENARIO_TRANSITIONS[state]

    if intent_name in next_steps:
        # This is an explicitly allowed transition
        return False

    if not can_leave:
        # This is not an allowed transition, and external transitions are disabled
        return True

    if (parent_intent_name(intent_name) == parent_intent_name(prev_intent_name)
            and parent_intent_name(intent_name) != intent_name):
        # We can leave, but aren't leaving
        return True

    # Ok, leaving is allowed
    return False


def is_internal(intent_name):
    return intent_name is not None and intent_name != parent_intent_name(intent_name)


def trim_name_prefix(intent_name, num_prefixes=-1):
    return intent_name.split('.', num_prefixes)[-1]


def compose_name(intent_name):
    return APP_INTENT_PREFIX + intent_name


def is_scenario(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX)


def is_microintent(intent_name):
    return intent_name is not None and intent_name.startswith(MICROINTENT_NAME_PREFIX)


def is_music_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music')


def is_music_play_intent(intent_name):
    return is_music_fairy_tale_intent(intent_name) or (
        intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_play')
    )


def is_music_fairy_tale_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_fairy_tale')


def is_music_sing_song_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_sing_song')


def is_bluetooth_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'bluetooth')


def is_news_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'get_news')


def is_reminders_intent(intent_name):
    return intent_name is not None and (
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'create_reminder') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'list_reminders'))


def is_todo_intent(intent_name):
    return intent_name is not None and (
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'create_todo') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'list_todo'))


def is_taxi_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'taxi') \
        and not intent_name.startswith(SCENARIOS_NAME_PREFIX + 'taxi_new')


def is_taxi_new_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'taxi_new')


def is_market_native(intent_name):
    return intent_name == MARKET_SCENARIOS_NAME_PREFIX + '_native'


def is_market_market_intent(intent_name):
    return intent_name == MARKET_SCENARIOS_NAME_PREFIX + '__market'


def is_market_intent(intent_name):
    return intent_name == MARKET_SCENARIOS_NAME_PREFIX


def is_market_intent_finished(intent_name):
    return is_market_intent(intent_name) and intent_name in MULTISTEP_SCENARIO_TRANSITIONS and \
        MULTISTEP_SCENARIO_TRANSITIONS[intent_name][0]


def is_market_beru_intent(intent_name):
    return intent_name in (MARKET_BERU_NAME_PREFIX, MARKET_NATIVE_BERU_NAME_PREFIX)


def is_how_much_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'how_much')


def is_how_much_internal_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'how_much__')


def is_recurring_purchase_intent(intent_name):
    return intent_name is not None and intent_name.startswith(RECURRING_PURCHASE_PREFIX)


def is_shopping_list_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SHOPPING_LIST_PREFIX) and not intent_name.endswith(SHOPPING_LIST_FIXLIST_SUFFIX)


def is_shopping_list_fixlist_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SHOPPING_LIST_PREFIX) and intent_name.endswith(SHOPPING_LIST_FIXLIST_SUFFIX)


def is_translate_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'translate')


def is_repeat_after_me_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'repeat_after_me')


def is_tv_stream_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'tv_stream')


def is_avia_intent(intent_name):
    return intent_name is not None and intent_name.startswith(AVIA_SCENARIOS_NAME_PREFIX)


def is_show_collection_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SHOW_COLLECTION_SCENARIOS_NAME_PREFIX)


def is_happy_new_year_intent(intent_name):
    return intent_name is not None and intent_name.startswith(HAPPY_NEW_YEAR_INTENT)


def is_happy_new_year_bloggers_intent(intent_name):
    return intent_name is not None and intent_name in HAPPY_NEW_YEAR_BLOGGERS


def is_negative_feedback(intent_name):
    return intent_name is not None and intent_name.startswith(FEEDBACK_NEGATIVE_PREFIX)


def is_positive_feedback(intent_name):
    return intent_name is not None and intent_name.startswith(FEEDBACK_POSITIVE_PREFIX)


def is_gc_feedback(intent_name):
    return intent_name is not None and intent_name.startswith(GC_FEEDBACK_PREFIX)


def is_feedback(intent_name):
    return is_positive_feedback(intent_name) or is_negative_feedback(intent_name) or is_gc_feedback(intent_name)


def get_negative_feedback_intent(response_intent_name):
    if response_intent_name is not None and response_intent_name.startswith(IMAGE_RECOGNIZER_INTENT):
        return FEEDBACK_NEGATIVE_IMAGES
    return FEEDBACK_NEGATIVE


def is_general_conversation(intent_name):
    return intent_name is not None and intent_name.startswith(GENERAL_CONVERSATION_PREFIX)


def is_gc_start(intent_name):
    return intent_name is not None and intent_name == PURE_GC


def is_gc_end(intent_name):
    return intent_name is not None and intent_name == PURE_GC_EXIT


def is_gc_only(intent_name):
    return intent_name.startswith(PURE_GC) and not is_gc_start(intent_name)


def is_allowed_in_gc(intent_name):
    # general_conversation or general_conversation_dummy
    if intent_name.startswith(GENERAL_CONVERSATION_PREFIX):
        return True
    return (is_microintent(intent_name) and LET_US_TALK_MICROINTENT != intent_name) or is_gc_only(intent_name)


def is_direct_gallery(intent_name):
    return intent_name is not None and intent_name.startswith(DIRECT_GALLERY_PREFIX)


def is_search(intent_name):
    return intent_name is not None and intent_name.startswith(SEARCH_PREFIX)


def is_open_site_or_app(intent_name):
    return intent_name is not None and intent_name.startswith(OPEN_SITE_OR_APP_PREFIX)


def should_cancel_listening(intent_name):
    return MICROINTENT_NAME_PREFIX + 'fast_cancel' == intent_name


def is_call_ellipsis(intent_name):
    return intent_name == CALL_ELLIPSIS


def is_messaging_ellipsis(intent_name):
    return intent_name.startswith(MESSAGING_ELLIPSIS_PREFIX)


def is_alarm_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'alarm')


def is_alarm_sound_old_microintent(intent_name):
    return intent_name is not None and (intent_name.startswith(QUASAR_MICROINTENTS_PREFIX + 'future_skill_set_alarm_sound') or
                                        intent_name.startswith(QUASAR_MICROINTENTS_PREFIX + 'future_skill_radio_alarm') or
                                        intent_name.startswith(QUASAR_MICROINTENTS_PREFIX + 'future_skill_what_music_on_alarm'))


def is_timer_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'timer')


def is_sleep_timer_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'sleep_timer')


def is_quasar_tv_specific(intent_name):
    """
    Whether the intent should be available only when the tv is plugged in
    """
    predicates = [
        is_quasar_video_screen_selection,
        is_quasar_gallery_navigation,
        is_quasar_screen_navigation,
        is_quasar_vertical_screen_navigation,
        is_quasar_video_payment_action,
        is_quasar_provider_authorization_action,
        is_quasar_video_details_opening_action,
    ]

    return any(p(intent_name) for p in predicates)


def is_quasar_faq(intent_name):
    return intent_name is not None and intent_name.startswith(QUASAR_MICROINTENTS_PREFIX)


def requires_quasar_tv_boost(intent_name):
    return is_quasar_tv_specific(intent_name) or intent_name in [QUASAR_GO_HOME]


def requires_quasar_video_mode_boost(intent_name):
    predicates = [
        is_quasar_tv_specific,
        is_player_intent,
        is_sound_intent,
        is_set_sound_level_intent,
        is_video_play_intent
    ]

    return any(p(intent_name) for p in predicates)


def requires_quasar_music_boost(intent_name):
    predicates = [
        is_music_intent,
        is_player_intent
    ]

    return any(p(intent_name) for p in predicates)


def is_quasar_gallery_navigation(intent_name):
    return intent_name in [
        QUASAR_GO_TO_THE_BEGINNING,
        QUASAR_GO_TO_THE_END,
        QUASAR_GO_FORWARD,
    ]


def is_quasar_screen_navigation(intent_name):
    # QUASAR_GO_HOME is also screen navigation, but without screen it switches to cancel
    return intent_name in [
        QUASAR_GO_BACKWARD,
    ]


def is_quasar_vertical_screen_navigation(intent_name):
    return intent_name in [
        QUASAR_GO_UP,
        QUASAR_GO_DOWN,
        QUASAR_GO_TOP
    ]


def is_quasar_video_payment_action(intent_name):
    return intent_name in [
        QUASAR_PAYMENT_CONFIRMED,
    ]


def is_quasar_provider_authorization_action(intent_name):
    return intent_name in [
        QUASAR_AUTHORIZE_VIDEO_PROVIDER,
    ]


def is_quasar_video_details_opening_action(intent_name):
    return intent_name == QUASAR_OPEN_CURRENT_VIDEO


def is_quasar_video_gallery_selection_by_text(intent_name):
    return intent_name == QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT


def is_quasar_ether_gallery_selection_by_text(intent_name):
    return intent_name == QUASAR_SELECT_ETHER_FROM_GALLERY_BY_TEXT


def is_quasar_channel_gallery_selection_by_text(intent_name):
    return intent_name == QUASAR_SELECT_CHANNEL_FROM_GALLERY_BY_TEXT


def is_quasar_video_screen_selection(intent_name):
    return intent_name == QUASAR_GO_TO_VIDEO_SCREEN


def is_item_selection(intent_name):
    return intent_name == INTERNAL_PREFIX + 'item_selection'


def is_video_play_intent(intent_name):
    return intent_name == SCENARIOS_NAME_PREFIX + 'video_play'


def is_player_intent(intent_name):
    return intent_name is not None and (intent_name.startswith(SCENARIOS_NAME_PREFIX + 'player') or
                                        intent_name.startswith(OPEN_OR_CONTINUE_INTENT))


def is_open_or_continue_intent(intent_name):
    return intent_name is not None and intent_name.startswith(OPEN_OR_CONTINUE_INTENT)


def is_player_resume_intent(intent_name):
    return intent_name in [
        OPEN_OR_CONTINUE_INTENT,
        PLAYER_CONTINUE_INTENT
    ]


def is_browser_read_page_pause_continue_intent(intent_name):
    return intent_name in [BROWSER_READ_PAGE_PAUSE_INTENT, BROWSER_READ_PAGE_CONTINUE_INTENT]


def is_player_replay_intent(intent_name):
    return SCENARIOS_NAME_PREFIX + 'player_replay' == intent_name


def is_sound_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'sound')


def is_sound_toggle_intent(intent_name):
    return intent_name is not None and (
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'sound_mute') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'sound_unmute')
    )


def is_set_sound_level_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'set_sound_level')


def is_stroka_intent(intent_name):
    return intent_name is not None and intent_name.startswith(STROKA_INTENT_NAME_PREFIX)


def is_navigator_intent(intent_name):
    return intent_name is not None and intent_name.startswith(NAVIGATOR_INTENT_NAME_PREFIX)


def requires_navigator_boost(intent_name):
    return intent_name in (
        'personal_assistant.scenarios.find_poi',
        'personal_assistant.scenarios.show_route',
    ) or is_navigator_intent(intent_name)


def is_music_recognizer_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_what_is_playing')


def is_image_onboarding_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'onboarding_image_search')


def is_image_recognizer_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'image_what_is_this')


def is_iot_repeat_phrase_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'quasar.iot.repeat_phrase')


def is_my_name(intent_name):
    return intent_name is not None and (
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'voiceprint_enroll') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'what_is_my_name') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'set_my_name')
    )


def is_voiceprint_remove(intent_name):
    return intent_name is not None and (
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'voiceprint_remove')
    )


def is_image_intent(intent_name):
    return is_image_onboarding_intent(intent_name) or is_image_recognizer_intent(intent_name)


def is_onboarding_continuation_intent(intent_name):
    return intent_name in (ONBOARDING_NEXT, ONBOARDING_CANCEL)


def is_onboarding_cancel_intent(intent_name):
    return intent_name == ONBOARDING_CANCEL


def is_alarm_stop_playing_intent(intent_name):
    return intent_name == ALARM_STOP_PLAYING_INTENT


def is_alarm_snooze_intent(intent_name):
    return intent_name == ALARM_SNOOZE_INTENT_ABS or intent_name == ALARM_SNOOZE_INTENT_REL


def is_alarm_sound_set_level_intent(intent_name):
    return intent_name == ALARM_SOUND_SET_LEVEL_INTENT


def is_timer_stop_playing_intent(intent_name):
    return intent_name == TIMER_STOP_PLAYING_INTENT


def is_stop_buzzing_intent(intent_name):
    return is_alarm_stop_playing_intent(intent_name) or is_timer_stop_playing_intent(intent_name) or is_alarm_snooze_intent(intent_name)


def is_image_what_is_this_specific_intent(intent_name):
    return intent_name in (IMAGE_WHAT_IS_THIS_OCR_INTENT, IMAGE_WHAT_IS_THIS_MARKET_INTENT, IMAGE_WHAT_IS_THIS_CLOTHES_INTENT, IMAGE_WHAT_IS_THIS_SIMILAR_INTENT, IMAGE_WHAT_IS_THIS_BARCODE_INTENT)


def should_force_voice_answer(intent_name, form):
    return is_music_sing_song_intent(intent_name) or (form and is_translate_intent(form.name) and form.repeat_voice.value)


def is_list_cancel_intent(intent_name):
    return intent_name == LIST_CANCEL_INTENT


def is_anaphora_source_intent(intent_name):
    return is_image_recognizer_intent(intent_name) or is_search(intent_name)


def is_anaphora_resolving_intent(intent_name):
    return intent_name == SCENARIOS_NAME_PREFIX + 'search_anaphoric'


def is_radio_onboarding_intent(intent_name):
    return intent_name is not None and intent_name.startswith(SCENARIOS_NAME_PREFIX + 'radio_play_onboarding')


def is_handcrafted_deaf_intent(intent_name):
    return intent_name == DEAF_MICROINTENT


INTENTS_WITH_SIDE_EFFECTS = {
    SCENARIOS_NAME_PREFIX + 'remember_named_location__ellipsis',
    SCENARIOS_NAME_PREFIX + 'connect_named_location_to_device__confirmation_yes',
    SCENARIOS_NAME_PREFIX + 'connect_named_location_to_device__confirmation_no',
    SCENARIOS_NAME_PREFIX + 'external_skill',
    SCENARIOS_NAME_PREFIX + 'external_skill__continue',
    SCENARIOS_NAME_PREFIX + 'cancel_todo',
    SCENARIOS_NAME_PREFIX + 'create_reminder',
    SCENARIOS_NAME_PREFIX + 'create_reminder__cancel',
    SCENARIOS_NAME_PREFIX + 'create_reminder__ellipsis',
    SCENARIOS_NAME_PREFIX + 'create_todo',
    SCENARIOS_NAME_PREFIX + 'create_todo__ellipsis',
    SCENARIOS_NAME_PREFIX + 'list_reminders__scroll_next',
    SCENARIOS_NAME_PREFIX + 'list_reminders__scroll_reset',
    SCENARIOS_NAME_PREFIX + 'voiceprint_enroll__collect_voice',
    SCENARIOS_NAME_PREFIX + 'voiceprint_enroll__finish',
    SCENARIOS_NAME_PREFIX + 'voiceprint_enroll',
    SCENARIOS_NAME_PREFIX + 'voiceprint_remove',
    SCENARIOS_NAME_PREFIX + 'voiceprint_remove__confirm',
    SCENARIOS_NAME_PREFIX + 'voiceprint_remove__finish',
    SCENARIOS_NAME_PREFIX + 'set_my_name',
    SCENARIOS_NAME_PREFIX + 'what_is_my_name',
    SCENARIOS_NAME_PREFIX + 'video_play',
    SCENARIOS_NAME_PREFIX + 'taxi_new_call_to_support',
    SCENARIOS_NAME_PREFIX + 'taxi_new_cancel__confirmation_yes',
    SCENARIOS_NAME_PREFIX + 'taxi_new_cancel__confirmation_no',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_yes',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_no',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order__confirmation_wrong',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order__change_payment_or_tariff',
    SCENARIOS_NAME_PREFIX + 'taxi_new_order__specify',
    SCENARIOS_NAME_PREFIX + 'taxi_new_show_driver_info',
    SCENARIOS_NAME_PREFIX + 'taxi_new_show_legal',
    SCENARIOS_NAME_PREFIX + 'taxi_new_status',
    SCENARIOS_NAME_PREFIX + 'taxi_new_disabled',
    SCENARIOS_NAME_PREFIX + 'taxi_order',
    SCENARIOS_NAME_PREFIX + 'tv_broadcast',
    SCENARIOS_NAME_PREFIX + 'shopping_list_add',
    SCENARIOS_NAME_PREFIX + 'shopping_list_delete_item',
    SCENARIOS_NAME_PREFIX + 'shopping_list_delete_all',
    SCENARIOS_NAME_PREFIX + 'shopping_list_login',
    SCENARIOS_NAME_PREFIX + 'shopping_list_show__add',
    SCENARIOS_NAME_PREFIX + 'shopping_list_show__delete_item',
    SCENARIOS_NAME_PREFIX + 'shopping_list_show__delete_index',
    SCENARIOS_NAME_PREFIX + 'shopping_list_show__delete_all',
    SCENARIOS_NAME_PREFIX + 'music_play',
    SCENARIOS_NAME_PREFIX + 'music_play_anaphora',
    SCENARIOS_NAME_PREFIX + 'music_ambient_sound',
    SCENARIOS_NAME_PREFIX + 'music_podcast',
    SCENARIOS_NAME_PREFIX + 'music_fairy_tale',
    SCENARIOS_NAME_PREFIX + 'music_sing_song',
    SCENARIOS_NAME_PREFIX + 'music_sing_song__next',
    SCENARIOS_NAME_PREFIX + 'music_what_is_playing__play',
    MARKET_CHECKOUT_PREFIX + "_confirmation",
    MARKET_CHECKOUT_PREFIX + "_yes_or_no",
    RECURRING_PURCHASE_PREFIX + '__checkout_yes_or_no',
    QUASAR_SELECT_VIDEO_FROM_GALLERY,
    QUASAR_GO_TO_VIDEO_SCREEN,
    QUASAR_OPEN_CURRENT_VIDEO,
    QUASAR_PAYMENT_CONFIRMED,
    QUASAR_AUTHORIZE_VIDEO_PROVIDER,
}


def is_with_side_effect(intent_name):
    additional = get_setting('INTENTS_WITH_SIDE_EFFECT', default='').split(',')

    if intent_name in INTENTS_WITH_SIDE_EFFECTS or intent_name in additional:
        return True

    return False


def is_potentially_search_intent(intent_name):
    # FIXME (a-sidorin@): Move hardcoded_music to is_music_intent()?
    return any((intent_name.startswith((SEARCH_PREFIX,
                                        EXTERNAL_SKILL_PREFIX,
                                        HAPPY_NEW_YEAR_INTENT,
                                        MARKET_SCENARIOS_NAME_PREFIX,
                                        RECURRING_PURCHASE_PREFIX,
                                        RADIO_PREFIX,
                                        SHOW_ROUTE_PREFIX,
                                        TV_PREFIX,
                                        MUSIC_RECOGNIZER_INTENT,
                                        FIND_POI_PREFIX)),
                is_video_play_intent(intent_name),
                is_music_intent(intent_name),
                is_navigator_intent(intent_name),
                is_news_intent(intent_name),
                intent_name in [SCENARIOS_NAME_PREFIX + 'hardcoded_music',
                                PLAYER_CONTINUE_INTENT,
                                PLAYER_NEXT_INTENT,
                                PLAYER_PREVIOUS_INTENT]
                ))


def is_heavy_scenario(intent_name):
    return any((intent_name.startswith((FIND_POI_PREFIX,
                                        AVIA_SCENARIOS_NAME_PREFIX,
                                        EXTERNAL_SKILL_PREFIX,
                                        SCENARIOS_NAME_PREFIX + 'remember_named_location',
                                        SCENARIOS_NAME_PREFIX + 'connect_named_location',
                                        SHOW_ROUTE_PREFIX,
                                        MARKET_SCENARIOS_NAME_PREFIX,
                                        RADIO_PREFIX,
                                        SCENARIOS_NAME_PREFIX + 'create_reminder',
                                        SCENARIOS_NAME_PREFIX + 'list_reminders',
                                        SEARCH_PREFIX,
                                        SCENARIOS_NAME_PREFIX + 'show_traffic',
                                        TV_PREFIX,
                                        SCENARIOS_NAME_PREFIX + 'get_weather'
                                        )),
                is_image_intent(intent_name),
                is_music_intent(intent_name),
                is_news_intent(intent_name),
                is_taxi_intent(intent_name),
                is_translate_intent(intent_name),
                is_player_intent(intent_name),
                is_video_play_intent(intent_name),
                intent_name in [ETHER_QUASAR_VIDEO_SELECT,
                                SCENARIOS_NAME_PREFIX + 'facts_apphost',
                                GENERAL_CONVERSATION,
                                QUASAR_SELECT_VIDEO_FROM_GALLERY,
                                QUASAR_GO_TO_VIDEO_SCREEN,
                                QUASAR_OPEN_CURRENT_VIDEO,
                                QUASAR_PAYMENT_CONFIRMED,
                                QUASAR_AUTHORIZE_VIDEO_PROVIDER
                                ]
                ))


def can_use_continue_stage(intent_name, req_info):
    if req_info.experiments['vins_use_continue'] is None:
        return False

    if is_with_side_effect(intent_name):
        return False

    if is_potentially_search_intent(intent_name) and req_info.experiments['vins_continue_on_search'] is None:
        return False

    if req_info.experiments['vins_continue_only_intent'] is not None:
        return req_info.experiments['vins_continue_only_intent'] == intent_name

    if req_info.experiments['vins_continue_heavy_only'] is not None:
        return is_heavy_scenario(intent_name)

    return True


def should_listen(intent_name, req_info, form):
    # Intent where we should never listen
    dont_listen_intents_predicates = (
        is_music_recognizer_intent,
        is_image_recognizer_intent,
        is_music_sing_song_intent,
        is_image_onboarding_intent,
        is_onboarding_cancel_intent,
        is_timer_stop_playing_intent,
        is_alarm_stop_playing_intent,
        is_alarm_snooze_intent,
        is_iot_repeat_phrase_intent,
        is_player_intent,
    )
    if any((predicate(intent_name) for predicate in dont_listen_intents_predicates)):
        return False

    # Microintents where we should never listen
    # TODO: move to microintents cfg?
    dont_listen_microintents = {
        'rude', 'goodbye', 'goodnight', "fast_cancel",
        'cancel', 'ok', 'user_reactions_you_are_welcome'
    }
    if is_microintent(intent_name) and trim_name_prefix(intent_name) in dont_listen_microintents:
        return False

    # Intents where we should always listen
    listen_intents_predicates = (
        lambda intent_name: intent_name == SCENARIOS_NAME_PREFIX + 'voiceprint_enroll',
        lambda intent_name: intent_name == INTERNAL_PREFIX + 'bugreport',
        lambda intent_name: intent_name == INTERNAL_PREFIX + 'bugreport__continue',
    )
    if any((predicate(intent_name) for predicate in listen_intents_predicates)):
        return True

    if clients.is_auto(req_info.app_info) and intent_name == EXTERNAL_SKILL_DEACTIVATE:
        return False

    if intent_name == SCENARIOS_NAME_PREFIX + 'voiceprint_enroll__collect_voice':
        return form is None or form.phrases_count is None or form.phrases_count.value is None

    return None


def is_username_autoinsert_allowed(intent_name, req_info):
    if intent_name is None:
        return False

    # 'music_personalization' experiment is designed to autoinsert user_name after specific
    # intents.
    # UPD: Also these intents should be whitelisted in username_auto_insert experiment
    if (req_info.experiments['music_personalization'] is not None or
            req_info.experiments['username_auto_insert'] is not None):
        if (intent_name.startswith(SCENARIOS_NAME_PREFIX + 'player_like') or
                intent_name.startswith(SCENARIOS_NAME_PREFIX + 'player_dislike') or
                intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_play')):
            return True

    # Otherwise, auto-insertion is forbidden outside of experiment.
    if req_info.experiments['username_auto_insert'] is None:
        return False

    # 'whitelisted' intents for initial tryout.
    return (
        intent_name.startswith(SEARCH_PREFIX) or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'find_poi') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'get_weather') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'get_my_location') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'music_sing_song') or
        intent_name.startswith(SCENARIOS_NAME_PREFIX + 'games_onboarding')
    )


def can_play_music(req_info):
    return req_info.device_state and any([player in req_info.device_state for player in ['music', 'audio_player']])


def is_playing_radio(req_info):
    device_state = req_info.device_state or {}
    return 'currently_playing' in device_state.get('radio', {})


def is_bluetooth_player_enabled(req_info):
    device_state = req_info.device_state or {}
    return 'currently_playing' in device_state.get('bluetooth', {})


def can_play_video(req_info):
    return req_info.device_state and 'video' in req_info.device_state


def are_player_commands_enabled(req_info):
    return can_play_music(req_info) or can_play_video(req_info) or is_playing_radio(req_info) or is_bluetooth_player_enabled(req_info) or clients.is_legatus(req_info.app_info)


def is_really_playing(req_info):
    device_state = req_info.device_state or {}
    video = not device_state.get('video', {}).get('currently_playing', {}).get('paused', True)
    music = (
        not device_state.get('music', {}).get('player', {}).get('pause', True) and
        'currently_playing' in device_state.get('music', {})
    )
    radio = (
        not device_state.get('radio', {}).get('player', {}).get('pause', True) and
        'currently_playing' in device_state.get('radio', {})
    )
    return video or music or radio


def can_change_sound(req_info):
    return (req_info.device_state and 'sound_level' in req_info.device_state) or clients.is_auto(req_info.app_info)


def can_toggle_sound(req_info):
    return can_change_sound(req_info) or clients.is_navigator(req_info.app_info)


def is_alarm_buzzing(req_info):
    return req_info.device_state and req_info.device_state.get('alarm_state', {}).get('currently_playing', False)


def is_timer_buzzing(req_info):
    return req_info.device_state and any(timer.get('currently_playing', False) for timer in
                                         req_info.device_state.get('timers', {}).get('active_timers', []))


def is_navigator_safe_mode(req_info):
    return is_navigator_projected_mode(req_info) or is_navigator_at_motrex(req_info)


def is_navigator_projected_mode(req_info):
    return (
        clients.is_navigator(req_info.app_info) and
        req_info.device_state and
        req_info.device_state.get('navigator', {}).get('projected_mode', False)
    )


def is_navigator_at_motrex(req_info):
    return (
        clients.is_navigator(req_info.app_info) and
        req_info.device_state and
        req_info.device_state.get('car_options', False) and
        req_info.device_state.get('car_options').get('type', '').lower() == 'standalone_navi' and
        req_info.device_state.get('car_options').get('model', '').lower() == 'mtrx_avn'
    )


def is_drive_faq(intent_name):
    return intent_name is not None and intent_name.startswith(DRIVE_MICROINTENTS_PREFIX)


def is_refuel_intent(intent_name):
    return intent_name is not None and intent_name.startswith(NAVIGATOR_INTENT_NAME_PREFIX + 'refuel')


@attr.s
class IntentExperiment(object):
    predicate = attr.ib()
    name = attr.ib()
    inverse = attr.ib(default=False)

    def intent_not_allowed(self, intent_name, req_info):
        return self.predicate(intent_name) and ((req_info.experiments[self.name] is None) ^ self.inverse)


def is_prohibited_intent(intent_name, req_info):
    if not req_info:
        return False
    if is_navigator_safe_mode(req_info):
        return not is_safe_for_navigator_safe_mode(intent_name, req_info)
    if clients.is_elari_watch(req_info.app_info):
        return not is_safe_for_elari_watch(intent_name)
    return False


def get_prohibition_error_intent(req_info):
    return PROHIBITION_ERROR_INTENT


EXP_ENABLE_PROTOCOL_SCENARIO_PREFIX = 'mm_enable_protocol_scenario='
EXP_DISABLE_PROTOCOL_SCENARIO_PREFIX = 'mm_disable_protocol_scenario='
EXP_DISABLE_PROTOCOL_VIDEO_SCENARIO = EXP_DISABLE_PROTOCOL_SCENARIO_PREFIX + 'Video'
EXP_ENABLE_NEWS_SCENARIO = EXP_DISABLE_PROTOCOL_SCENARIO_PREFIX + 'News'
EXP_IRRELEVANT_INTENTS = 'vins_irrelevant_intents'
EXP_ADD_IRRELEVANT_INTENTS_PREFIX = 'vins_add_irrelevant_intents='
EXP_REMOVE_IRRELEVANT_INTENTS_PREFIX = 'vins_remove_irrelevant_intents='
EXP_EXTERNAL_ONLY_INTENTS_PREFIX = 'vins_external_only_intents='
EXP_FORBIDDEN_INTENTS = 'forbidden_intents'
EXP_ENABLE_MORDOVIA_VIDEO_SELECTION_SCENARIO = EXP_ENABLE_PROTOCOL_SCENARIO_PREFIX + 'MordoviaVideoSelection'


# TODO(a-square): put music_play here when launching HollywoodMusic on all devices
EXPERIMENT_TO_FORBIDDEN_INTENTS = {
    EXP_ENABLE_MORDOVIA_VIDEO_SELECTION_SCENARIO: [
        ETHER_QUASAR_VIDEO_SELECT,
        "personal_assistant.scenarios.ether_show",
    ],
}

QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS = {
    EXP_DISABLE_PROTOCOL_VIDEO_SCENARIO: [
        'personal_assistant.scenarios.quasar.select_video_from_gallery_by_text',
        'personal_assistant.scenarios.quasar.select_video_from_gallery',
    ],
    EXP_ENABLE_NEWS_SCENARIO: [
        'personal_assistant.scenarios.get_news'  # https://st.yandex-team.ru/EXPERIMENTS-50748
    ]
}

FORBIDDEN_INTENTS = set()


SKIP_RELEVANT_INTENTS = {
    'route': SkipRelevantIntents(experiment=EXP_ENABLE_PROTOCOL_SCENARIO_PREFIX+'Route', list_intents=(
        {'personal_assistant.scenarios.show_route__taxi'},
        {
            'personal_assistant.scenarios.show_route__ellipsis',
            'personal_assistant.scenarios.show_route__show_route_on_map_spec',
        },
    )),
}


SKIP_RELEVANT_INTENTS_FOR_SESSION = {
    'route': SkipRelevantIntents(experiment=EXP_ENABLE_PROTOCOL_SCENARIO_PREFIX+'Route', list_intents=(
        'personal_assistant.scenarios.remember_named_location',
        'personal_assistant.scenarios.remember_named_location__ellipsis',
    )),
}


def skip_relevant_intents(scenario_id, semantic_frames, experiments):
    skip_intents = SKIP_RELEVANT_INTENTS.get(scenario_id)
    if not skip_intents or not semantic_frames:
        return False
    if skip_intents.experiment and experiments[skip_intents.experiment] is None:
        return False
    semantic_frame_intents = {semantic_frame['name'] for semantic_frame in semantic_frames}
    for intents in skip_intents.list_intents:
        if semantic_frame_intents.issuperset(intents):
            logger.info('skip_relevant_intents for scenario "%s" because semantic_frame_intents %r',
                        scenario_id, semantic_frame_intents)
            return True
    return False


def get_sessions_for_skip_relevants_intents(req_info):
    skip_intents = SKIP_RELEVANT_INTENTS_FOR_SESSION.get(req_info.scenario_id)
    if not skip_intents:
        return None
    if skip_intents.experiment and req_info.experiments[skip_intents.experiment] is None:
        return None
    return skip_intents.list_intents


def get_quasar_forbidden_intents(req_info):
    forbidden_intents = set()

    for experiment, intents in EXPERIMENT_TO_FORBIDDEN_INTENTS.iteritems():
        if req_info.experiments[experiment] is not None:
            forbidden_intents.update(intents)

    if clients.is_smart_speaker(req_info.app_info) or clients.is_tv_device(req_info.app_info):
        for experiment, intents in QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS.iteritems():
            if req_info.experiments[experiment] is None:
                forbidden_intents.update(intents)

    return forbidden_intents


def get_forbidden_intents(req_info):
    forbidden_intents = FORBIDDEN_INTENTS.copy()

    if not req_info:
        return forbidden_intents

    if req_info.experiments[EXP_FORBIDDEN_INTENTS]:
        forbidden_intents_from_exp = req_info.experiments[EXP_FORBIDDEN_INTENTS].split(',')
        forbidden_intents.update(forbidden_intents_from_exp)

    if req_info.experiments['disable_quasar_forbidden_intents'] is None:
        forbidden_intents.update(get_quasar_forbidden_intents(req_info))

    return forbidden_intents


def gen_forbidden_intents_experiment(req_info):
    return ','.join(get_forbidden_intents(req_info))


def product_scenario_name(req_info):
    if not req_info:
        return None

    scenario_id = req_info.scenario_id
    if scenario_id in SCENARIO_RELEVANT_INTENTS:
        return SCENARIO_RELEVANT_INTENTS[scenario_id].default_product_scenario_name

    return None


def intent_has_irrelevant_name(intent_name):
    return intent_name and intent_name.startswith(IRRELEVANT_ERROR_INTENT_PREFIX)


def intent_in_irrelevant_list(intent_name, req_info):
    if not req_info:
        return False

    if req_info.scenario_id:
        return (
            req_info.scenario_id not in SCENARIO_RELEVANT_INTENTS or (
                intent_name not in SCENARIO_RELEVANT_INTENTS[req_info.scenario_id].intents and
                intent_name not in SCENARIO_ALWAYS_RELEVANT_INTENTS
            )
        )

    if is_irrelevant_intent_by_exp(intent_name, req_info.experiments):
        return True

    if is_relevant_intent_by_exp(intent_name, req_info.experiments):
        return False

    if clients.is_smart_speaker(req_info.app_info):
        if intent_name in VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS:
            if req_info.experiments['vins_onboarding_relevant_again'] is None:
                return True

    if clients.is_smart_speaker(req_info.app_info) or clients.is_tv_device(req_info.app_info):
        if intent_name in VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS_AND_TV_DEVICES:
            return True

    if req_info.experiments['vins_sound_commands_relevant_again'] is None:
        if intent_name in VINS_IRRELEVANT_INTENTS_SOUND_COMMANDS:
            return True

    if req_info.experiments['vins_pause_commands_relevant_again'] is None:
        if intent_name in VINS_IRRELEVANT_INTENTS_PAUSE_COMMANDS:
            return True

    return intent_name in VINS_IRRELEVANT_INTENTS


def is_irrelevant_intent(intent_name, req_info):
    return intent_has_irrelevant_name(intent_name) or intent_in_irrelevant_list(intent_name, req_info)


def is_irrelevant_intent_by_exp(intent_name, experiments):
    if experiments[EXP_IRRELEVANT_INTENTS]:
        intents = urllib.unquote(experiments[EXP_IRRELEVANT_INTENTS])
        if intent_name in intents.split(','):
            return True

    for exp, _ in experiments.items():
        exp = urllib.unquote(exp)
        if exp.startswith(EXP_ADD_IRRELEVANT_INTENTS_PREFIX):
            intents = exp[len(EXP_ADD_IRRELEVANT_INTENTS_PREFIX):]
            if intent_name in intents.split(','):
                return True
    return False


def filter_exp_by_prefix(experiments, prefix):
    for exp, _ in experiments.items():
        exp = urllib.unquote(exp)
        if exp.startswith(prefix):
            yield exp, exp[len(prefix):]


def get_intents_from_exp(experiments, prefix):
    data = set()
    for exp, suffix in filter_exp_by_prefix(experiments, prefix):
        data.update(suffix.split(','))

    return data


def is_relevant_intent_by_exp(intent_name, experiments):
    intents = get_intents_from_exp(experiments, EXP_REMOVE_IRRELEVANT_INTENTS_PREFIX)
    return intent_name in intents


def allow_external_only_frame(intent_name, req_info):
    intents = get_intents_from_exp(req_info.experiments, EXP_EXTERNAL_ONLY_INTENTS_PREFIX)
    frames = req_info.semantic_frames or []
    semantic_frame_intents = {sf['name'] for sf in frames}
    is_external_only = intent_name in intents

    if is_external_only and intent_name not in semantic_frame_intents:
        return False

    return True


def get_irrelevant_intent(intent_name, req_info):
    # Return intent for irrelevant situation.
    # Due to the way intent is implemented, some of them require special slots,
    # so the function returns specific irrelevant for specific intents.
    # For all others, the generic IRRELEVANT_ERROR_INTENT_DEFAULT is used
    # Please, if possible, use only IRRELEVANT_ERROR_INTENT_DEFAULT
    return IRRELEVANT_ERROR_INTENTS.get(intent_name, IRRELEVANT_ERROR_INTENT_DEFAULT)
