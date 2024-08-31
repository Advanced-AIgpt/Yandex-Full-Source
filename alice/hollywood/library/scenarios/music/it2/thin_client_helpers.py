import json

from alice.megamind.protos.scenarios.directives_pb2 import TAudioPlayDirective
from google.protobuf import json_format


EXPERIMENTS = [
    'hw_music_thin_client',
]

EXPERIMENTS_RADIO = [
    'new_music_radio_nlg',
]

EXPERIMENTS_PLAYLIST = [
    'hw_music_thin_client_playlist'
]

EXPERIMENTS_HLS = [
    'hw_music_thin_client_use_hls'
]


def assert_audio_play_directive(directives, title=None, sub_title=None, has_callbacks=True, offset_ms=None, is_hls=False, set_pause=False, track_id=None, incognito=False):
    assert len(directives) == 1

    audio_play = directives[0].AudioPlayDirective
    assert audio_play.Name == 'music'
    if track_id:
        assert audio_play.Stream.Id == track_id
    else:
        assert audio_play.Stream.Id

    assert audio_play.SetPause == set_pause
    if is_hls:
        assert audio_play.Stream.StreamFormat == TAudioPlayDirective.TStream.TStreamFormat.HLS
    else:
        assert audio_play.Stream.StreamFormat == TAudioPlayDirective.TStream.TStreamFormat.MP3
    assert audio_play.Stream.Url.startswith('https://')
    if has_callbacks:
        assert audio_play.Callbacks.HasField('OnPlayStartedCallback')
        assert audio_play.Callbacks.HasField('OnPlayStoppedCallback')
        assert audio_play.Callbacks.HasField('OnPlayFinishedCallback')
        assert audio_play.Callbacks.HasField('OnFailedCallback')
        for callback in [audio_play.Callbacks.OnPlayStartedCallback,
                         audio_play.Callbacks.OnPlayStoppedCallback,
                         audio_play.Callbacks.OnPlayFinishedCallback,
                         audio_play.Callbacks.OnFailedCallback]:
            if 'events' in callback.Payload:
                for event in callback.Payload['events']:
                    for key in event:
                        if incognito:
                            assert event[key]['incognito'] == incognito
                        else:
                            assert 'incognito' not in event[key]

    assert audio_play.ScenarioMeta['owner'] == 'music'
    assert audio_play.ScreenType == TAudioPlayDirective.EScreenType.Music

    assert audio_play.HasField('AudioPlayMetadata')
    if title:
        assert audio_play.AudioPlayMetadata.Title == title
    if sub_title:
        assert audio_play.AudioPlayMetadata.SubTitle == sub_title

    if offset_ms is not None:
        assert audio_play.Stream.OffsetMs >= offset_ms

    return audio_play


def assert_glagol_metadata(glagol_metadata, object_type, object_id, shuffled=False, repeat_mode=getattr(TAudioPlayDirective.TAudioPlayMetadata.ERepeatMode, "None")):
    music_metadata = glagol_metadata.MusicMetadata

    assert music_metadata.Type == TAudioPlayDirective.TAudioPlayMetadata.EContentType.Value(object_type)
    assert music_metadata.Id == object_id
    if shuffled is None:
        assert not music_metadata.HasField("Shuffled")
    else:
        assert music_metadata.Shuffled == shuffled
    if repeat_mode is None:
        assert not music_metadata.HasField("RepeatMode")
    else:
        assert music_metadata.RepeatMode == repeat_mode


def get_first_track_object(analytics_info):
    for obj in analytics_info.Objects:
        if obj.Id == 'music.first_track_id':
            return obj
    return None


def get_first_track_id(analytics_info):
    first_track_obj = get_first_track_object(analytics_info)
    return first_track_obj.FirstTrack.Id


def get_callback(callbacks, callback_name):
    return {
        'music_thin_client_on_started': callbacks.OnPlayStartedCallback,
        'music_thin_client_on_stopped': callbacks.OnPlayStoppedCallback,
        'music_thin_client_on_finished': callbacks.OnPlayFinishedCallback,
        'music_thin_client_on_failed': callbacks.OnFailedCallback,
    }[callback_name]


def get_callback_from_reset_add_effect(response_reset_add, callback_name=None, effect_index=0):
    callback_pb = response_reset_add.Effects[effect_index].Callback
    if callback_name:
        assert callback_pb.Name == callback_name
    result = proto_to_dict(callback_pb)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'HollywoodMusic'  # TODO(vitvlkv) Avoid this hack, HOLLYWOOD-269
    })
    return result


def get_recovery_callback_from_reset_add(response_reset_add, callback_name=None):
    callback_pb = response_reset_add.RecoveryAction.Callback
    if callback_name:
        assert callback_pb.Name == callback_name
    result = proto_to_dict(callback_pb)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'HollywoodMusic'  # TODO(vitvlkv) Avoid this hack, HOLLYWOOD-269
    })
    return result


def proto_to_dict(proto_message):
    json_str = json_format.MessageToJson(proto_message)
    return json.loads(json_str)


def prepare_server_action_data(thin_player_callback):
    result = proto_to_dict(thin_player_callback)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'HollywoodMusic'  # TODO(vitvlkv) Avoid this hack, HOLLYWOOD-269
    })
    return result
