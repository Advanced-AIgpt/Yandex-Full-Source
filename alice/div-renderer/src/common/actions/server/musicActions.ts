import {
    TStringSlot,
} from '../../../protos/alice/megamind/protos/common/frame';
import { Directive } from '../index';
import { createSemanticFrameAction, createSemanticFrameActionTypeSafe } from './index';

//region Shuffle
export function centaurShuffleOn(): Directive {
    return createSemanticFrameAction(
        {
            player_shuffle_semantic_frame: {
                disable_nlg: {
                    bool_value: true,
                },
            },
        },
        'Centaur music shuffle on',
        'Centaur music shuffle on',
    );
}

export function centaurShuffleOff(): Directive {
    return createSemanticFrameAction(
        {
            player_unshuffle_semantic_frame: {
                disable_nlg: {
                    bool_value: true,
                },
            },
        },
        'Centaur music shuffle off',
        'Centaur music shuffle off',
    );
}

//endregion

//region Repeat

export enum EnumRepeatMode {
    Unknown = 0,
    None = 1,
    One = 2,
    All = 3,
}

export function centaurActionRepeat(mode: EnumRepeatMode): Directive {
    return createSemanticFrameAction(
        {
            player_repeat_semantic_frame: {
                disable_nlg: {
                    bool_value: true,
                },
                mode: {
                    enum_value: mode,
                },
            },
        },
        `Centaur music repeat ${mode}`,
        `Centaur music repeat ${mode}`,
    );
}

interface IMusicPlayProps {
    startFromTrackId?: string;
    playlist?: string;
    objectId: string;
    objectType: string;
    disableNlg?: boolean;
}

function createStringSlot({
    StringValue = undefined,
    ActionRequestValue = undefined,
    EpochValue = undefined,
    NewsTopicValue = undefined,
    SpecialAnswerInfoValue = undefined,
    SpecialPlaylistValue = undefined,
    VideoActionValue = undefined,
    VideoContentTypeValue = undefined,
    VideoSelectionActionValue = undefined,
}: Partial<TStringSlot>): TStringSlot {
    return {
        StringValue,
        ActionRequestValue,
        EpochValue,
        NewsTopicValue,
        SpecialAnswerInfoValue,
        SpecialPlaylistValue,
        VideoActionValue,
        VideoContentTypeValue,
        VideoSelectionActionValue,
    };
}

export function centaurMusicPlay({
    startFromTrackId,
    playlist,
    objectId,
    objectType,
    disableNlg,
}: IMusicPlayProps) {
    return createSemanticFrameActionTypeSafe({
        MusicPlaySemanticFrame: {
            StartFromTrackId: startFromTrackId ? createStringSlot({
                StringValue: startFromTrackId,
            }) : undefined,
            Playlist: playlist ? createStringSlot({
                StringValue: playlist,
            }) : undefined,
            ObjectId: objectId ? createStringSlot({
                StringValue: objectId,
            }) : undefined,
            ObjectType: objectType ? {
                EnumValue: Number(objectType),
            } : undefined,
            DisableNlg: disableNlg ? {
                BoolValue: disableNlg,
            } : undefined,
        },
    },
    'Change track',
    'Change track');
}

//endregion
