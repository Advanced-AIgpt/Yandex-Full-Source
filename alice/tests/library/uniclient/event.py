import alice.megamind.protos.common.events_pb2 as events_pb2

from . import analytics
from .proto_wrapper import ProtoWrapper
from .typed_semantic_frame import frame


def _to_dict(data):
    return data.dict() if isinstance(data, ProtoWrapper) else data


class _Event(ProtoWrapper):
    proto_cls = events_pb2.TEvent


class ImageInput(_Event):
    def __init__(self, img_url, capture_mode):
        super().__init__(
            type=events_pb2.EEventType.image_input,
            payload=dict(img_url=img_url, capture_mode=capture_mode),
        )


class ServerAction(_Event):
    def __init__(self, name, ignore_answer=False, **payload):
        super().__init__(
            type=events_pb2.EEventType.server_action,
            name=name,
            ignore_answer=ignore_answer,
            payload=payload,
        )


class UpdateForm(ServerAction):
    def __init__(self, payload):
        super().__init__(name='update_form', **payload)


class SemanticFrame(ServerAction):
    def __init__(self, typed_semantic_frame, analytics):
        super().__init__(
            name='@@mm_semantic_frame',
            typed_semantic_frame=_to_dict(typed_semantic_frame),
            analytics=_to_dict(analytics),
        )

    def dict(self):
        proto = super().dict()

        tsf = proto.get('payload', {}).get('typed_semantic_frame', {})
        none_keys = [key for key, value in tsf.items() if value is None]
        for key in none_keys:
            tsf[key] = {}

        return proto


class _MusicSemanticFrame(SemanticFrame):
    def __init__(self, typed_semantic_frame):
        super().__init__(
            typed_semantic_frame,
            analytics.Scenario(purpose='play_music'),
        )


class MusicPlay(_MusicSemanticFrame):
    def __init__(self, object_id, object_type, start_from_track_id, offset_sec):
        super().__init__(
            frame.MusicPlaySemanticFrame(
                object_id=dict(string_value=object_id),
                object_type=dict(enum_value=object_type),
                disable_nlg=dict(bool_value=True),
                start_from_track_id=dict(string_value=start_from_track_id),
                offset_sec=dict(double_value=offset_sec),
            )
        )


class PlayerNextTrack(_MusicSemanticFrame):
    def __init__(self, set_pause=None):
        super().__init__(
            frame.PlayerNextTrackSemanticFrame(
                set_pause=dict(bool_value=set_pause),
            )
        )


class PlayerPrevTrack(_MusicSemanticFrame):
    def __init__(self, set_pause=None):
        super().__init__(
            frame.PlayerPrevTrackSemanticFrame(
                set_pause=dict(bool_value=set_pause),
            )
        )


class PlayerReplay(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerReplaySemanticFrame()
        )


class PlayerContinue(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerContinueSemanticFrame()
        )


class PlayerShuffle(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerShuffleSemanticFrame()
        )


class PlayerRewind(_MusicSemanticFrame):
    def __init__(self, time, rewind_type):
        super().__init__(
            frame.PlayerRewindSemanticFrame(
                time=dict(units_time_value=time),
                rewind_type=dict(rewind_type_value=rewind_type),
            )
        )


class PlayerRepeat(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerRepeatSemanticFrame()
        )


class PlayerWhatIsPlaying(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerWhatIsPlayingSemanticFrame()
        )


class PlayerLike(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerLikeSemanticFrame()
        )


class PlayerDislike(_MusicSemanticFrame):
    def __init__(self):
        super().__init__(
            frame.PlayerDislikeSemanticFrame()
        )


class SetupRcuStatus(SemanticFrame):
    def __init__(self, status):
        super().__init__(
            frame.SetupRcuStatusSemanticFrame(
                status=dict(enum_value=status),
            ),
            analytics.RemoteControl(
                purpose='setup_rcu_status',
            )
        )


class SetupRcuAutoStatus(SemanticFrame):
    def __init__(self, status):
        super().__init__(
            frame.SetupRcuAutoStatusSemanticFrame(
                status=dict(enum_value=status),
            ),
            analytics.RemoteControl(
                purpose='setup_rcu_auto_status',
            )
        )


class SetupRcuAdvancedStatus(SemanticFrame):
    def __init__(self, status):
        super().__init__(
            frame.SetupRcuAdvancedStatusSemanticFrame(
                status=dict(enum_value=status),
            ),
            analytics.RemoteControl(
                purpose='setup_rcu_advanced_status',
            )
        )


class SetupRcuCheckStatus(SemanticFrame):
    def __init__(self, status):
        super().__init__(
            frame.SetupRcuCheckStatusSemanticFrame(
                status=dict(enum_value=status),
            ),
            analytics.RemoteControl(
                purpose='setup_rcu_check_status',
            )
        )


class SetupRcuManualStart(SemanticFrame):
    def __init__(self):
        super().__init__(
            frame.SetupRcuManualStartSemanticFrame(),
            analytics.RemoteControl(
                purpose='setup_rcu_manual_start',
            )
        )


class SetupRcuAutoStart(SemanticFrame):
    def __init__(self, tv_model):
        super().__init__(
            frame.SetupRcuAutoStartSemanticFrame(
                tv_model=dict(string_value=tv_model),
            ),
            analytics.RemoteControl(
                purpose='setup_rcu_auto_start',
            )
        )


class CentaurCollectCards(SemanticFrame):
    def __init__(self, carousel_id):
        super().__init__(
            frame.CentaurCollectCardsSemanticFrame(
                carousel_id=dict(string_value=carousel_id),
            ),
            analytics.SmartSpeaker(
                purpose='centaur_collect_cards',
            )
        )


class CentaurCollectMainScreen(SemanticFrame):
    def __init__(self, widget_gallery_position):
        super().__init__(
            frame.CentaurCollectMainScreenSemanticFrame(
                widget_gallery_position=dict(widget_position_value=widget_gallery_position),
            ),
            analytics.SmartSpeaker(
                purpose='centaur_collect_main_screen',
            )
        )


class CentaurCollectWidgetGallery(SemanticFrame):
    def __init__(self, column, row):
        super().__init__(
            frame.CentaurCollectWidgetGallerySemanticFrame(
                column=dict(num_value=column),
                row=dict(num_value=row),
            ),
            analytics.SmartSpeaker(
                purpose='open_widget_gallery_from_centaur_main_screen',
            )
        )


class CentaurAddWidgetFromGallery(SemanticFrame):
    def __init__(self, column, row, widget_config_data_slot):
        super().__init__(
            frame.CentaurAddWidgetFromGallerySemanticFrame(
                column=dict(num_value=column),
                row=dict(num_value=row),
                widget_config_data_slot=dict(widget_config_data_value=widget_config_data_slot),
            ),
            analytics.SmartSpeaker(
                purpose='add_widget_from_gallery',
            )
        )


class RequestTechnicalSupport(SemanticFrame):
    def __init__(self):
        super().__init__(
            frame.RequestTechnicalSupportSemanticFrame(),
            analytics.RemoteControl(
                purpose='request_technical_support',
            )
        )


class OnboardingGetGreetings(SemanticFrame):
    def __init__(self):
        super().__init__(
            frame.OnboardingGetGreetingsSemanticFrame(),
            analytics.SearchApp(
                purpose='get_greetings',
            )
        )
