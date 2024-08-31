import { DivStateBlock, FixedSize, ImageBlock, WrapContentSize } from 'divcard2';
import { getStaticS3Asset } from '../../../helpers/assets';
import { IStatePlace, setStateActionInAllPlaces } from '../../../../../common/actions/div';
import { DualScreenStates } from '../../../components/DualScreen/DualScreen';
import {
    MUSIC_PLAYER_SECONDARY_BLOCK,
    MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
    MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
} from '../constants';

interface ILikeButtonProps {
    isLiked: boolean;
    size?: number;
    options?: Omit<ConstructorParameters<typeof DivStateBlock>[0], 'div_id' | 'states'>;
    trackId: string;
    color?: string;
}

const TRACK_LIKE_LIKED = 'liked';
const TRACK_LIKE_NOT_LIKED = 'unliked';

const likePlaces: IStatePlace[] = [
    {
        name: 'music_horizontal_cover',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateHorizontal,
            MUSIC_PLAYER_SECONDARY_BLOCK,
            MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
        ],
    },
    {
        name: 'music_horizontal_list',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateHorizontal,
            MUSIC_PLAYER_SECONDARY_BLOCK,
            MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
        ],
    },
    {
        name: 'music_vertical_cover',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateVertical,
            MUSIC_PLAYER_SECONDARY_BLOCK,
            MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
        ],
    },
    {
        name: 'music_vertical_list',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateVertical,
            MUSIC_PLAYER_SECONDARY_BLOCK,
            MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
        ],
    },
];

export default function LikeButton({ isLiked, size = 48, options, color = '#fff', trackId }: ILikeButtonProps) {
    const likeId = `like_for_track_${trackId}`;
    return new DivStateBlock({
        div_id: likeId,
        width: new WrapContentSize(),
        ...options,
        default_state_id: isLiked ? TRACK_LIKE_LIKED : TRACK_LIKE_NOT_LIKED,
        states: [
            {
                state_id: TRACK_LIKE_LIKED,
                div: new ImageBlock({
                    width: new FixedSize({ value: size }),
                    height: new FixedSize({ value: size }),
                    tint_color: color,
                    alpha: 1,
                    image_url: getStaticS3Asset('music/like_on.png'),
                    actions: [
                        ...setStateActionInAllPlaces({
                            places: likePlaces,
                            state: [likeId, TRACK_LIKE_NOT_LIKED],
                            logPrefix: 'set_not_liked_',
                        }),
                    ],
                }),
            },
            {
                state_id: TRACK_LIKE_NOT_LIKED,
                div: new ImageBlock({
                    width: new FixedSize({ value: size }),
                    height: new FixedSize({ value: size }),
                    alpha: 0.5,
                    image_url: getStaticS3Asset('music/like_off.png'),
                    actions: [
                        ...setStateActionInAllPlaces({
                            places: likePlaces,
                            state: [likeId, TRACK_LIKE_LIKED],
                            logPrefix: 'set_liked_',
                        }),
                    ],
                }),
            },
        ],
    });
}
