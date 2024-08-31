# NOTE: if you need a new event - just add a class for it and add the
# class to the list of all events.


YT_TYPE_BOOLEAN = 'boolean'
YT_TYPE_DOUBLE = 'double'
YT_TYPE_INT64 = 'int64'
YT_TYPE_STRING = 'string'
YT_TYPE_DATETIME = 'datetime'


class EventSynchronizeState:
    """Event when (last) event SynchronizeState is received."""
    NAME = 'synchronize_state_timestamp_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventSynchronizeStateFinished:
    """
    Event when event SynchronizeState processing is finished (and resume to processing user events).
    Contain duration/lag to synchronize_state_timestamp
    """
    NAME = 'synchronize_state_finished_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventStartProcessVoiceInput:
    """
    Event when begin processing VoiceInput event (real event can be suspended if client not wait ending SynchronizeState process).
    Contain duration/lag to synchronize_state_timestamp_sec
    Also this is base timestamp for events like EventEndOfUtterance
    """
    NAME = 'start_process_voice_input_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsUaasStart:
    NAME = 'vins_uaas_start_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsUaasEnd:
    NAME = 'vins_uaas_end_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventAsrContextsReady:
    NAME = 'contexts_ready_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsSessionLoadEnd:
    NAME = 'vins_session_load_end_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsPersonalDataStart:
    NAME = 'vins_personal_data_start_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsPersonalDataEnd:
    NAME = 'vins_personal_data_end_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventFirstAsrResult:
    """Event for first asr result (include empty partials)."""
    NAME = 'first_asr_result_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventEndOfUtterance:
    """Event when EOU signal from ASR is received."""
    NAME = 'end_of_utterance_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastClassifyPartial:
    """Event of the classification result."""
    NAME = 'last_classify_partial_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastPartial:
    """Event of the last non-EOU partial."""
    NAME = 'last_partial_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastScorePartial:
    """Event of the scoring result."""
    NAME = 'last_score_partial_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulPartial:
    """Event for asr result with text same as asr_end result(eou)."""
    NAME = 'useful_partial_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestAsr:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with asr data)"""
    NAME = 'useful_vins_prepare_request_asr'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestPersonalData:
    """Duration for stage after getting useful partual and real request to vins (waiting personal data results)"""
    NAME = 'useful_vins_prepare_request_personal_data'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestClassify:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with biometry classify data)"""
    NAME = 'useful_vins_prepare_request_classify'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestNotificationState:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with notification state)"""
    NAME = 'useful_vins_prepare_request_notification_state'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestMemento:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with memento)"""
    NAME = 'useful_vins_prepare_request_memento'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestContacts:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with contacts)"""
    NAME = 'useful_vins_prepare_request_contacts'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestLaas:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with laas)"""
    NAME = 'useful_vins_prepare_request_laas'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestMusic:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with music data)"""
    NAME = 'useful_vins_prepare_request_music'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestYabio:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with yabio data)"""
    NAME = 'useful_vins_prepare_request_yabio'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsPrepareRequestSession:
    """Duration for stage after getting useful partual and real request to vins (waiting request part with session)"""
    NAME = 'useful_vins_prepare_request_session'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsRequest:
    NAME = 'useful_vins_request_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulVinsRequestDuration:
    """Duration of the last run request (no side-effects) from VINS."""
    NAME = 'useful_vins_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsResponse:
    """Event when VINS response is being sent to the client."""
    NAME = 'vins_response_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventResultVinsRunResponseIsReady:
    """
    Event when last Run (no side-effects) response from VINS was
    received.
    """
    NAME = 'result_vins_run_response_is_ready_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastVinsRunRequestDurationSec:
    """Duration of the last run request (no side-effects) from VINS."""
    NAME = 'last_vins_run_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastVinsPreparingRequestDurationSec:
    """Duration of waiting responses from music&yabio after receiving result from asr"""
    NAME = 'last_vins_preparing_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventStartVinsRequestEOU:
    """Duration of the starting request to vins with eou=true from VINS."""
    NAME = 'start_vins_request_eou'
    YT_TYPE = YT_TYPE_DOUBLE


class EventFinishVinsRequestEOU:
    """Duration of the finish request to vins with eou=true from VINS."""
    NAME = 'finish_vins_request_eou'
    YT_TYPE = YT_TYPE_DOUBLE


class EventStartVinsApplyRequest:
    """Lag from begin processing VoiceInput to sending apply request to VINS."""
    NAME = 'start_vins_apply_request_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastVinsApplyRequestDurationSec:
    """Duration of the apply request from VINS."""
    NAME = 'last_vins_apply_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventStartExecuteVinsDirectives:
    NAME = 'start_execute_vins_directives_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventFinishExecuteVinsDirectives:
    NAME = 'finish_execute_vins_directives_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventLastVinsFullRequestDurationSec:
    """Duration of the last run request with side-effects request from VINS."""
    NAME = 'last_vins_full_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventMeanVinsRequestDurationSec:
    """Average duration of all run (no side-effects) requests from VINS."""
    NAME = 'mean_vins_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventMeanVinsPreparingRequestDurationSec:
    """Average duration of waiting responses from music&yabio after receiving result from asr."""
    NAME = 'mean_vins_preparing_request_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsRequestCount:
    """Count of all run (no side-effects) requests from VINS."""
    NAME = 'vins_request_count'
    YT_TYPE = YT_TYPE_INT64


class EventHasApplyVinsRequest:
    """Presence of the apply request from VINS."""
    NAME = 'has_apply_vins_request'
    YT_TYPE = YT_TYPE_BOOLEAN


class EventLastVinsRunRequestIntentName:
    """Intent name of the last run request from VINS."""
    NAME = 'last_vins_run_request_intent_name'
    YT_TYPE = YT_TYPE_STRING


class EventHasVinsFullResultOnEOU:
    """Was VINS response ready before EOU has been received."""
    NAME = 'has_vins_full_result_on_eou'
    YT_TYPE = YT_TYPE_BOOLEAN


class EventVinsRunWaitAfterEOUDurationSec:
    """Duration between EOU and VINS run response."""
    NAME = 'vins_run_wait_after_eou_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsRunDelayAfterEOUDurationSec:
    """Duration between EOU and VINS run response."""
    NAME = 'vins_run_delay_after_eou_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsWaitAfterEOUDurationSec:
    """Duration between EOU and VINS run or apply response."""
    NAME = 'vins_wait_after_eou_duration_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventGetSpotterValidationResultStart:
    NAME = 'get_spotter_validation_result_start_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventGetSpotterValidationResultEnd:
    NAME = 'get_spotter_validation_result_end_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventVinsResponseSent:
    """Vins response sendin to user delay from request start."""
    NAME = 'vins_response_sent_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventTtsStart:
    NAME = 'tts_start_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventTtsCacheResponse:
    NAME = 'tts_cache_response_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventTtsCacheSuccess:
    NAME = 'tts_cache_success'
    YT_TYPE = YT_TYPE_BOOLEAN


class EventFirstTTSChunkSec(object):
    """Duration between event processing start and first TTS chunk."""
    NAME = 'first_tts_chunk_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUsefulResponseForUser:
    """Duration between first tts chunk (if has tts) or vins response and event processing start."""
    NAME = 'useful_response_for_user_evage'
    YT_TYPE = YT_TYPE_DOUBLE


class EventRequestId:
    """Request id used for the current request."""
    NAME = 'request_id'
    YT_TYPE = YT_TYPE_STRING


class EventEndOfSpeechSec:
    """Event of the end of speech signal."""
    NAME = 'end_of_speech_sec'
    YT_TYPE = YT_TYPE_DOUBLE


class EventUttid:
    """
    Uttid - unique identifier of the audio file used to shoot at the
    Uniproxy.
    """
    NAME = 'uttid'
    YT_TYPE = YT_TYPE_STRING


class EventEpoch:
    """
    Server-time epoch when the request processing was started.
    """
    NAME = 'epoch'
    YT_TYPE = YT_TYPE_DATETIME


class EventAppType:
    """
    App-type, used to distinguish between different datasets.
    """
    NAME = 'app_type'
    YT_TYPE = YT_TYPE_STRING


class EventDatasetId:
    """
    Dataset ID, used to distinguish between different datasets.
    """
    NAME = 'dataset_id'
    YT_TYPE = YT_TYPE_STRING


ALL_EVENTS = [
    EventSynchronizeState,
    EventSynchronizeStateFinished,
    EventVinsUaasStart,
    EventVinsUaasEnd,
    EventStartProcessVoiceInput,
    EventAsrContextsReady,
    EventVinsSessionLoadEnd,
    EventVinsPersonalDataStart,
    EventVinsPersonalDataEnd,
    EventFirstAsrResult,
    EventEndOfUtterance,
    EventLastClassifyPartial,
    EventLastPartial,
    EventLastScorePartial,
    EventUsefulPartial,
    EventUsefulVinsPrepareRequestAsr,
    EventUsefulVinsPrepareRequestClassify,
    EventUsefulVinsPrepareRequestMusic,
    EventUsefulVinsPrepareRequestYabio,
    EventUsefulVinsPrepareRequestSession,
    EventUsefulVinsRequest,
    EventUsefulVinsRequestDuration,
    EventVinsResponse,
    EventVinsWaitAfterEOUDurationSec,
    EventResultVinsRunResponseIsReady,
    EventLastVinsRunRequestDurationSec,
    EventStartVinsRequestEOU,
    EventFinishVinsRequestEOU,
    EventStartVinsApplyRequest,
    EventLastVinsApplyRequestDurationSec,
    EventStartExecuteVinsDirectives,
    EventFinishExecuteVinsDirectives,
    EventLastVinsFullRequestDurationSec,
    EventMeanVinsRequestDurationSec,
    EventVinsRequestCount,
    EventHasApplyVinsRequest,
    EventLastVinsRunRequestIntentName,
    EventGetSpotterValidationResultStart,
    EventGetSpotterValidationResultEnd,
    EventVinsResponseSent,
    EventTtsStart,
    EventTtsCacheResponse,
    EventTtsCacheSuccess,
    EventFirstTTSChunkSec,
    EventUsefulResponseForUser,
    EventRequestId,
    EventEndOfSpeechSec,
    EventUttid,
    EventEpoch,
    EventAppType,
    EventDatasetId
]

BY_NAME = {ev.NAME : ev for ev in ALL_EVENTS}

TTS_STAGE = [
    EventTtsStart,
    EventTtsCacheResponse,
    EventTtsCacheSuccess,
    EventFirstTTSChunkSec,
    EventUsefulResponseForUser,
    EventEndOfSpeechSec,
]

TTS_STAGE_BY_NAME = {ev.NAME : ev for ev in TTS_STAGE}
