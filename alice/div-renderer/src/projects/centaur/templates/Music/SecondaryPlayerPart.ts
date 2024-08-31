import {
    ContainerBlock, DivAppearanceSetTransition, DivFadeTransition, DivSlideTransition,
    DivStateBlock,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    Template,
} from 'divcard2';
import { compact } from 'lodash';
import { getImageCoverByLink } from '../FullMusic/GetImageCoverByLink';
import {
    MUSIC_PLAYER_SECONDARY_BLOCK,
    MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
    MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
} from './constants';
import { IMusicTrackInfo } from './types';
import { getS3Asset } from '../../helpers/assets';
import {
    offsetFromEdgeOfScreen,
    whiteBackgroundOpacity10,
} from '../../style/constants';
import { IColorSet } from '../../style/colorSet';
import { IRequestState } from '../../../../common/types/common';
import PartList from '../../components/DualScreen/partComponents/PartList';
import { getPlaylistItem } from './common';
import PartBasicTopCenterBottom from '../../components/DualScreen/partComponents/PartBasicTopCenterBottom';
import LikeButton from './components/LikeButton';
import DislikeButton from './components/DislikeButton';

interface ISecondaryPlayerPartProps {
    coverURI: string | Template;
    playlist: IMusicTrackInfo[];
    colorSet: IColorSet;
    trackId: string;
    playlistId: string;
    objectType: string;
    requestState: IRequestState;
    isLiked?: boolean;
    isDisliked?: boolean;
}

const COVER_SIZE = 480;
const COVER_BORDER_RADIUS = 24;

const animationTime = 400;

export function SecondaryPlayerPart({
    coverURI,
    playlist,
    colorSet,
    trackId,
    playlistId,
    objectType,
    requestState,
    isLiked,
    isDisliked,
}: ISecondaryPlayerPartProps) {
    return new DivStateBlock({
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize({ weight: 1 }),
        div_id: MUSIC_PLAYER_SECONDARY_BLOCK,
        transition_animation_selector: 'data_change',
        default_state_id: MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
        states: compact([
            playlist.length > 0 && {
                state_id: MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
                animation_in: {
                    name: 'set',
                    items: [
                        {
                            name: 'translate',
                            duration: animationTime,
                            start_delay: animationTime / 2,
                            start_value: 1,
                            end_value: 0,
                            interpolator: 'ease_out',
                        },
                        {
                            name: 'fade',
                            duration: animationTime,
                            start_delay: animationTime / 2,
                            start_value: 0,
                            end_value: 1,
                            interpolator: 'ease_out',
                        },
                    ],
                },
                animation_out: {
                    name: 'set',
                    items: [
                        {
                            name: 'translate',
                            duration: animationTime,
                            start_value: 0,
                            end_value: 1,
                            interpolator: 'ease_in',
                        },
                        {
                            name: 'fade',
                            duration: animationTime,
                            start_value: 1,
                            end_value: 0,
                            interpolator: 'ease_in',
                        },
                    ],
                },
                div: PartList({
                    id: 'music_playlist',
                    colorSet,
                    items: playlist.map(el => getPlaylistItem({
                        subtitle: el.artists?.filter(artist => artist.isComposer === false).map(artist => artist.name).join(', ') || undefined,
                        objectId: playlistId,
                        startFromTrackId: el.id,
                        objectType,
                        title: el.title,
                        imageUrl: el.altImageUrl ? getImageCoverByLink(el.altImageUrl, '80x80') : '',
                        isCurrent: el.id === trackId,
                        requestState,
                        isLiked: el.isLiked,
                    })),
                    requestState,
                })[0],
            },
            {
                state_id: MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
                animation_in: {
                    name: 'set',
                    items: [
                        {
                            name: 'fade',
                            duration: animationTime,
                            start_value: 0,
                            start_delay: animationTime / 2,
                            end_value: 1,
                            interpolator: 'ease_out',
                        },
                        {
                            name: 'translate',
                            duration: animationTime,
                            start_value: -1,
                            start_delay: animationTime / 2,
                            end_value: 0,
                            interpolator: 'ease_out',
                        },
                    ],
                },
                animation_out: {
                    name: 'set',
                    items: [
                        {
                            name: 'fade',
                            duration: animationTime,
                            start_value: 1,
                            end_value: 0,
                            interpolator: 'ease_in',
                        },
                        {
                            name: 'translate',
                            duration: animationTime,
                            start_value: 0,
                            end_value: -1,
                            interpolator: 'ease_in',
                        },
                    ],
                },
                div: PartBasicTopCenterBottom({
                    middleDivItems: [
                        new ContainerBlock({
                            width: new FixedSize({ value: COVER_SIZE }),
                            height: new FixedSize({ value: COVER_SIZE }),
                            background: whiteBackgroundOpacity10,
                            content_alignment_vertical: 'center',
                            content_alignment_horizontal: 'center',
                            alignment_horizontal: 'center',
                            border: {
                                corner_radius: COVER_BORDER_RADIUS,
                            },
                            orientation: 'overlap',
                            items: [
                                new ImageBlock({
                                    image_url: getS3Asset('music/no_image.png'),
                                    width: new FixedSize({ value: 120 }),
                                    height: new FixedSize({ value: 138 }),
                                }),
                                new ImageBlock({
                                    image_url: coverURI,
                                    width: new FixedSize({ value: COVER_SIZE }),
                                    height: new FixedSize({ value: COVER_SIZE }),
                                    border: {
                                        corner_radius: COVER_BORDER_RADIUS,
                                    },
                                }),
                            ],
                        }),
                    ],
                    bottomSize: new FixedSize({ value: offsetFromEdgeOfScreen * 2 }),
                    topSize: new FixedSize({ value: offsetFromEdgeOfScreen * 2 }),
                    ...(typeof isLiked !== 'undefined' && typeof isDisliked !== 'undefined' && {
                        bottomDivItems: [
                            DislikeButton({ isDisliked, options: { margins: { right: 32 } } }),
                            LikeButton({ isLiked, trackId }),
                        ],
                        bottomDivOptions: {
                            content_alignment_horizontal: 'center',
                            orientation: 'horizontal',
                            margins: {
                                bottom: offsetFromEdgeOfScreen,
                            },
                        },
                    }),
                    containerOptions: {
                        id: `cover_for_${trackId}`,
                        transition_in: new DivAppearanceSetTransition({
                            items: [
                                new DivFadeTransition({
                                    duration: animationTime,
                                    interpolator: 'ease_in',
                                }),
                                new DivSlideTransition({
                                    duration: animationTime,
                                    edge: 'right',
                                    distance: { value: offsetFromEdgeOfScreen },
                                }),
                            ],
                        }),
                        transition_out: new DivAppearanceSetTransition({
                            items: [
                                new DivFadeTransition({
                                    duration: animationTime,
                                    interpolator: 'ease_out',
                                }),
                                new DivSlideTransition({
                                    duration: animationTime,
                                    edge: 'left',
                                    distance: { value: offsetFromEdgeOfScreen },
                                }),
                            ],
                        }),
                    },
                })[0],
            },
        ]),
    });
}
