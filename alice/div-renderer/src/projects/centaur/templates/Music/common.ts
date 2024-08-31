import {
    ContainerBlock,
    Div,
    FixedSize,
    IDivAction,
    ImageBackground,
    ImageBlock, MatchParentSize,
    SolidBackground,
    TextBlock,
} from 'divcard2';
import { colorBlackOpacity70, offsetFromEdgeOfScreen } from '../../style/constants';
import { compact } from 'lodash';
import EmptyDiv from '../../components/EmptyDiv';
import { text28m } from '../../style/Text/Text';
import { directivesAction } from '../../../../common/actions';
import { centaurMusicPlay } from '../../../../common/actions/server/musicActions';
import MusicPlaying from './MusicPlaying/MusicPlaying';
import { IRequestState } from '../../../../common/types/common';
import LikeButton from './components/LikeButton';

interface IGetPlaylistItemProps {
    title: string;
    subtitle?: string;
    objectId: string;
    objectType: string;
    startFromTrackId?: string;
    isCurrent?: boolean;
    imageUrl: string;
    imageBackground?: string;
    requestState: IRequestState;
    isLiked?: boolean;
}

function CoverWithBackground(imageUrl: string, background: string, action: IDivAction, isCurrent: boolean, requestState: IRequestState) {
    return new ContainerBlock({
        width: new FixedSize({ value: 80 }),
        height: new FixedSize({ value: 80 }),
        alignment_vertical: 'top',
        actions: [action],
        margins: {
            right: 24,
        },
        border: {
            corner_radius: 40,
        },
        content_alignment_horizontal: 'center',
        content_alignment_vertical: 'center',
        background: compact([
            new SolidBackground({
                color: background,
            }),
        ]),
        orientation: 'overlap',
        items: compact([
            new ImageBlock({
                image_url: imageUrl,
                width: new FixedSize({ value: 48 }),
                height: new FixedSize({ value: 48 }),
            }),
            isCurrent && new EmptyDiv({
                width: new FixedSize({ value: 80 }),
                height: new FixedSize({ value: 80 }),
                background: [
                    new SolidBackground({
                        color: colorBlackOpacity70,
                    }),
                ],
            }),
            isCurrent && MusicPlaying(requestState),
        ]),
    });
}

function CoverWithoutBackground(imageUrl: string, action: IDivAction, isCurrent: boolean, requestState: IRequestState) {
    return new ContainerBlock({
        width: new FixedSize({ value: 80 }),
        height: new FixedSize({ value: 80 }),
        alignment_vertical: 'top',
        content_alignment_horizontal: 'center',
        content_alignment_vertical: 'center',
        actions: [action],
        margins: {
            right: 24,
        },
        border: {
            corner_radius: 20,
        },
        background: compact([
            new ImageBackground({
                image_url: imageUrl,
            }),
            isCurrent && new SolidBackground({
                color: colorBlackOpacity70,
            }),
        ]),
        items: [
            isCurrent ? MusicPlaying(requestState) : new EmptyDiv(),
        ],
    });
}

export function getPlaylistItem({
    isCurrent = false,
    objectType,
    subtitle,
    objectId,
    title,
    startFromTrackId,
    imageUrl,
    imageBackground,
    requestState,
    isLiked,
}: IGetPlaylistItemProps): {
    options: Partial<ConstructorParameters<typeof ContainerBlock>[0]>;
    items: Div[];
} {
    const openAction = {
        log_id: 'play_music_click',
        url: directivesAction(centaurMusicPlay({
            disableNlg: true,
            objectType,
            objectId,
            startFromTrackId,
        })),
    };

    return {
        options: {
            orientation: 'horizontal',
            paddings: {
                top: 28,
                left: offsetFromEdgeOfScreen,
                bottom: 28,
                right: offsetFromEdgeOfScreen,
            },
            content_alignment_vertical: 'center',
        },
        items: compact([
            imageBackground ?
                CoverWithBackground(imageUrl, imageBackground, openAction, isCurrent, requestState) :
                CoverWithoutBackground(imageUrl, openAction, isCurrent, requestState),
            new ContainerBlock({
                width: new MatchParentSize({ weight: 1 }),
                actions: [openAction],
                items: compact([
                    new TextBlock({
                        ...text28m,
                        text: title,
                    }),
                    subtitle && new TextBlock({
                        ...text28m,
                        text: subtitle,
                        alpha: 0.5,
                    }),
                ]),
            }),
            typeof isLiked !== 'undefined' && LikeButton({ isLiked, trackId: startFromTrackId || '' }),
        ]),
    };
}
