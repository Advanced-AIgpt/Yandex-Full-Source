import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import { Layer } from '../../common/layers';
import { ACTION_CLOSE_MUSIC } from './constants';
import { ContainerBlock, FixedSize, ImageBlock, MatchParentSize, SolidBackground, TextBlock } from 'divcard2';
import { getImageCoverByLink } from '../FullMusic/GetImageCoverByLink';
import PartBasicTopCenterBottom from '../../components/DualScreen/partComponents/PartBasicTopCenterBottom';
import { text42m, title54m } from '../../style/Text/Text';
import { PlayPausePlayerButton } from '../FullMusic/components/PlayerControls/components/PlayPausePlayerButton';
import PartList from '../../components/DualScreen/partComponents/PartList';
import { getPlaylistItem } from './common';
import { IMusicDivProps } from './types';

export default function RadioDiv({
    colorSet,
    playlist,
    coverURI,
    objectType,
    trackId,
    requestState,
    imageBackground,
    title,
}: IMusicDivProps) {
    return CloseButtonWrapper({
        div: DualScreen({
            requestState,
            firstDiv: PartBasicTopCenterBottom({
                topDivItems: [
                    new TextBlock({
                        ...text42m,
                        alpha: 0.5,
                        text: 'Радио',
                        width: new MatchParentSize({ weight: 1 }),
                        max_lines: 1,
                    }),
                ],
                middleDivItems: [
                    new ContainerBlock({
                        width: new FixedSize({ value: 348 }),
                        height: new FixedSize({ value: 348 }),
                        content_alignment_vertical: 'center',
                        content_alignment_horizontal: 'center',
                        border: {
                            corner_radius: 174,
                        },
                        background: imageBackground ? [
                            new SolidBackground({
                                color: imageBackground,
                            }),
                        ] : undefined,
                        items: [
                            new ImageBlock({
                                image_url: coverURI,
                                width: new FixedSize({ value: 228 }),
                                height: new FixedSize({ value: 228 }),
                            }),
                        ],
                    }),
                    new TextBlock({
                        ...title54m,
                        text: title,
                        margins: {
                            top: 24,
                            bottom: 20,
                        },
                        text_alignment_horizontal: 'center',
                    }),
                    PlayPausePlayerButton({
                        colorSet,
                        requestState,
                    }),
                ],
                middleDivOptions: {
                    content_alignment_horizontal: 'center',
                },
                topSize: new FixedSize({ value: 100 }),
                bottomSize: new FixedSize({ value: 100 }),
            }),
            secondDiv: PartList({
                id: 'radio_list',
                colorSet,
                items: playlist.map(el => getPlaylistItem({
                    title: el.title,
                    imageUrl: el.altImageUrl ? getImageCoverByLink(el.altImageUrl, '80x80') : '',
                    objectType,
                    objectId: el.id,
                    isCurrent: el.id === trackId,
                    imageBackground: el.imageBackground,
                    requestState,
                })),
                requestState,
            }),
            mainColor1: colorSet.mainColor1,
            mainColor: colorSet.mainColor,
            inverseOnVertical: true,
        }),
        layer: Layer.CONTENT,
        actions: [
            {
                log_id: 'close_music_session',
                url: ACTION_CLOSE_MUSIC,
            },
        ],
    });
}
