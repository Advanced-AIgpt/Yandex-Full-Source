import {
    ContainerBlock,
    Div, DivAppearanceSetTransition,
    DivFadeTransition,
    FixedSize,
    MatchParentSize,
    Template,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { PlayPausePlayerButton } from '../FullMusic/components/PlayerControls/components/PlayPausePlayerButton';
import { text28m, text42m, title54m } from '../../style/Text/Text';
import {
    colorWhiteOpacity50,
    offsetFromEdgeOfPartOfScreen,
    offsetFromEdgeOfScreen,
} from '../../style/constants';
import EmptyDiv from '../../components/EmptyDiv';
import MusicProgressDivCustom from '../../divCustoms/MusicProgress';
import { NextPlayerButton } from '../FullMusic/components/PlayerControls/components/NextPlayerButton';
import { PreviousPlayerButton } from '../FullMusic/components/PlayerControls/components/PreviousPlayerButton';
import { MoreButton } from './components/MoreButton';
import { NAlice } from '../../../../protos';
import { IColorSet } from '../../style/colorSet';
import { ShuffleButton } from './components/ShuffleButton';
import { RepeatButton } from './components/RepeatButton';
import { MUSIC_ICON_SIZE, MUSIC_PLAYER_BOTTOM_PART_HEIGHT, MUSIC_PLAYER_TOP_PART_HEIGHT } from './constants';
import { IRequestState } from '../../../../common/types/common';

export function MainPlayerPart({
    header,
    title,
    colorSet,
    isShuffle,
    repeatMode,
    renderPlaylistButton = false,
    playlistName,
    requestState,
}: {
    header: string | Template;
    title: string | Template;
    playlistName: string | Template;
    colorSet: Readonly<IColorSet>;
    isShuffle: boolean | null;
    repeatMode: NAlice.NData.ERepeatMode | null,
    requestState: IRequestState,
    renderPlaylistButton?: boolean,
}): Div {
    return new ContainerBlock({
        height: new MatchParentSize({ weight: 1 }),
        width: new MatchParentSize({ weight: 1 }),
        paddings: {
            top: offsetFromEdgeOfPartOfScreen,
            left: offsetFromEdgeOfScreen,
            bottom: (
                offsetFromEdgeOfPartOfScreen -
                // Поправка на отступ для анимации кнопки
                MUSIC_ICON_SIZE * 0.2
            ),
            right: offsetFromEdgeOfPartOfScreen,
        },
        items: [
            new ContainerBlock({
                orientation: 'horizontal',
                content_alignment_vertical: 'center',
                width: new MatchParentSize({ weight: 1 }),
                height: new FixedSize({ value: MUSIC_PLAYER_TOP_PART_HEIGHT - offsetFromEdgeOfPartOfScreen }),
                items: compact([
                    new TextBlock({
                        ...text42m,
                        alpha: 0.5,
                        text: playlistName,
                        width: new MatchParentSize({ weight: 1 }),
                        max_lines: 1,
                    }),
                    renderPlaylistButton && MoreButton(colorSet),
                ]),
            }),
            new ContainerBlock({
                height: new MatchParentSize({ weight: 1 }),
                margins: {
                    right: offsetFromEdgeOfScreen - offsetFromEdgeOfPartOfScreen,
                },
                content_alignment_vertical: 'center',
                id: `text_for_track_${title}${header}`,
                transition_in: new DivAppearanceSetTransition({
                    items: [
                        new DivFadeTransition({
                            duration: 400,
                            interpolator: 'ease_in',
                        }),
                    ],
                }),
                transition_out: new DivAppearanceSetTransition({
                    items: [
                        new DivFadeTransition({
                            duration: 400,
                            interpolator: 'ease_out',
                        }),
                    ],
                }),
                items: [
                    new TextBlock({
                        ...title54m,
                        width: new MatchParentSize(),
                        text_alignment_horizontal: 'center',
                        text: title,
                        max_lines: 4,
                        margins: {
                            bottom: 12,
                        },
                    }),
                    new TextBlock({
                        ...text42m,
                        alpha: 0.5,
                        text_alignment_horizontal: 'center',
                        text: header,
                        max_lines: 2,
                    }),
                ],
            }),
            new ContainerBlock({
                height: new FixedSize({ value: MUSIC_PLAYER_BOTTOM_PART_HEIGHT - offsetFromEdgeOfPartOfScreen }),
                margins: {
                    right: offsetFromEdgeOfScreen - offsetFromEdgeOfPartOfScreen,
                },
                content_alignment_vertical: 'bottom',
                items: [
                    new ContainerBlock({
                        orientation: 'horizontal',
                        width: new MatchParentSize({ weight: 1 }),
                        items: [
                            new TextBlock({
                                ...text28m,
                                text: '00:00',
                                text_color: colorWhiteOpacity50,
                                alignment_vertical: 'center',
                                margins: {
                                    right: 20,
                                },
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                extensions: [
                                    {
                                        id: 'music-player-progress',
                                    },
                                ],
                            }),
                            MusicProgressDivCustom(),
                            new TextBlock({
                                ...text28m,
                                text: '00:00',
                                text_color: colorWhiteOpacity50,
                                alignment_vertical: 'center',
                                margins: {
                                    left: 20,
                                },
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                extensions: [
                                    {
                                        id: 'music-player-duration',
                                    },
                                ],
                            }),
                        ],
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        content_alignment_vertical: 'center',
                        width: new MatchParentSize({ weight: 1 }),
                        margins: {
                            top: 24,
                        },
                        items: compact([
                            isShuffle !== null && new ContainerBlock({
                                // todo: Remove ContainerBlock when fix bug MORDAANDROID-1035
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                items: [ShuffleButton(colorSet, isShuffle)],
                            }),
                            new EmptyDiv({
                                width: new MatchParentSize({ weight: 1 }),
                            }),
                            PreviousPlayerButton({
                                colorSet,
                            }),
                            PlayPausePlayerButton({
                                colorSet,
                                requestState,
                            }),
                            NextPlayerButton({
                                colorSet,
                            }),
                            new EmptyDiv({
                                width: new MatchParentSize({ weight: 1 }),
                            }),
                            repeatMode !== null && new ContainerBlock({
                                // todo: Remove ContainerBlock when fix bug MORDAANDROID-1035
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                items: [RepeatButton(colorSet, repeatMode)],
                            }),
                        ]),
                    }),
                ],
            }),
        ],
    });
}
