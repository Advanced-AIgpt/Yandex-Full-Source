package ru.yandex.alice.kronstadt.scenarios.video_call

// call control
const val MESSENGER_CALL_TO_FRAME = "alice.messenger_call.call_to"
const val MESSENGER_ACCEPT_INCOMING_CALL_FRAME = "alice.messenger_call.accept_incoming_call"
const val MESSENGER_DISCARD_INCOMING_CALL_FRAME = "alice.messenger_call.stop_incoming_call"
const val MESSENGER_HANGUP_CALL_FRAME = "alice.messenger_call.stop_current_call"
const val PHONE_CALL_TO_FRAME = "alice.phone_call"
const val VIDEO_CALL_TO_FRAME = "alice.video_call_to"
const val VIDEO_CALL_INCOMING_FRAME = "alice.video_call_incoming"

const val CONTACT_BOOK_ITEM_NAME = "item_name"
const val VIDEO_ENABLED_SLOT = "video_enabled"

// mic & video control
const val VIDEO_CALL_MUTE_MIC = "alice.video_call.mute_mic"
const val VIDEO_CALL_UNMUTE_MIC = "alice.video_call.unmute_mic"

// endpoint updates
const val ENDPOINT_STATE_UPDATES_FRAME = "alice.endpoint.state.updates"

// callbacks
const val VIDEO_CALL_LOGIN_FAILED_FRAME = "alice.video_call_login_failed"
const val VIDEO_CALL_OUTGOING_ACCEPTED_FRAME = "alice.video_call_outgoing_accepted"
const val VIDEO_CALL_OUTGOING_FAILED_FRAME = "alice.video_call_outgoing_failed"
const val VIDEO_CALL_INCOMING_ACCEPT_FAILED_FRAME = "alice.video_call_incoming_accept_failed"

// widget data
const val CENTAUR_COLLECT_MAIN_SCREEN = "alice.centaur.collect_main_screen"
const val CENTAUR_COLLECT_WIDGET_GALLERY = "alice.centaur.collect_widget_gallery"

// contact book
const val PHONE_CALL_OPEN_ADDRESS_BOOK = "alice.phone_call.open_address_book"
const val SET_FAVORITES = "alice.video_call_set_favorites"

// Intents
const val CALL_TO_INTENT = "alice_scenarios.video_call_to"
const val CALL_INCOMING_INTENT = "alice_scenarios.video_call_incoming"
const val HANGUP_CALL_INTENT = "alice_scenarios.hangup_video_call"
const val DECLINE_CALL_INTENT = "alice_scenarios.decline_video_call"
const val ACCEPT_CALL_INTENT = "alice_scenarios.accept_video_call"
const val PROVIDER_LOGIN_INTENT = "alice_scenarios.video_call_provider_login"
const val STATE_UPDATES_INTENT = "alice.endpoint.state.updates"
const val OPEN_CONTACTS_BOOK_INTENT = "alice.phone_call.open_address_book"