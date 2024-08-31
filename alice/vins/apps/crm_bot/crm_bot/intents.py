# coding: utf-8

APP_INTENT_PREFIX = 'crm_bot.'

SIMPLE_FINAL_INTENTS_ = [  # autoadd feedback request ending after these
    'bonus_bb_activate__continuation',
    'bonus_bb_not_recieved_b',
    'bonus_bb_not_recieved_cb',
    'bonus_bb_not_recieved_cc',
    'bonus_greet',
    'bonus_sbrf_thankyou_how_to',
    'bonus_sbrf_thankyou_get',
    'bonus_sbrf_thankyou_not_available',
    'bonus_ya_plus',
    'bonus_problem_ya_plus',
    'credit_how_to',
    'credit_not_available',
    'credit_conditions',
    'delivery_deadline',
    'delivery_price',
    'delivery_free',
    'delivery_cityname',
    'delivery_nooption',
    "order_cancel__has_account",
    "order_cancel_accident",
    "order_cancel_accident__already_paid",
    "order_return_fine__forbidden",
    "order_return_fine__7days",
    "order_return_fine__14days",
    "order_got_defective__after_a_while__no_sc",
    "order_got_defective__right_after",
    "order_return_complications__no_auth",
    'payment_issues_aa',
    'payment_issues_ab',
    'payment_issues_b',
    'payment_issues_cb',
    'payment_issues_cc',
    'payment_issues_cd',
    'payment_issues_ce',
    'payment_issues_caa',
    'payment_legal_entity',
    'payment_return_policy_after_cancellation__card',
    'payment_return_policy_after_cancellation__credit',
    'payment_return_policy_after_cancellation__sbrf_thanks',
    'payment_return_policy_after_return__card',
    'payment_return_policy_after_return__credit',
    'payment_return_policy_after_return__sbrf_thanks',
    'payment_return_policy_after_return__when_arrived',
    'payment_options_online',
    'payment_options_online_how_to',
    'payment_options_offline',
    'pickup_location_find',
    'search_is_legal',
    'search_warranty_duration',
    'search_warranty_duration__no_info',
    'search_warranty_location',
    'search_warranty_paper',
    'order_change_goods',
    'order_how_to',
    'other_b2b',
    'out_of_stock',
    'search_details__no_info',
    'search__cant_find'
]

FEEDBACK_WHITELIST_ = [  # can activate feedback scenarios after these
    'search',
    'order_status',
    'order_status__data',
    'order_status__continuation',
    'order_status__where_is_pickup_location',
    'order_cancel_for_me_finish',
    'order_cancel_we_did'
]

GREET_BLACKLIST_ = [  # never greet in the begining of these
    'bye',
    'bye_narrow'
]

VERBOSE_REDIRECT_INTENTS_ = [
    'bonus_bb_apply',
    'bonus_bb_get',
    'bonus_problem',
    'bonus_problem_promocode',
    'bonus_promocode',
    'bonus_sbrf_thankyou_problem',
    'call_me',
    'choose_for_me',
    'complaints_call_center',
    'complaints_cancel',
    'complaints_delivery_deadline',
    'complaints_delivery_price',
    'complaints_delivery_rescheduled',
    'complaints_other',
    'complaints_packaging',
    'delivery_incomplete_package',
    'delivery_procedure',
    'delivery_tracking',
    'delivery_wrong_package',
    'garbage',
    'help_me_configure',
    'lk_edit',
    'order_change',
    'order_error',
    'order_prolong_storage',
    'payment_return_problem',
    'search_details'
]

FINAL_INTENTS = [APP_INTENT_PREFIX+'scenarios.'+i for i in SIMPLE_FINAL_INTENTS_]

FEEDBACK_WHITELIST = [APP_INTENT_PREFIX+'scenarios.'+i for i in FEEDBACK_WHITELIST_]
FEEDBACK_WHITELIST.extend(FINAL_INTENTS)

GREET_BLACKLIST = [APP_INTENT_PREFIX+'scenarios.'+i for i in GREET_BLACKLIST_]

VERBOSE_REDIRECT_INTENTS = [APP_INTENT_PREFIX+'scenarios.'+i for i in VERBOSE_REDIRECT_INTENTS_]


import logging
logger = logging.getLogger(__file__)


def is_crmbot_intent(intent_name):
    return intent_name.startswith(APP_INTENT_PREFIX)


def is_final_intent(intent_name):
    return intent_name in FINAL_INTENTS


def can_greet(intent_name):
    return intent_name not in GREET_BLACKLIST
