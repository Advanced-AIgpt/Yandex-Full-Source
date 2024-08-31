import { ContainerBlock, Div, GalleryBlock, TextBlock } from 'divcard2';
import { NAlice } from '../../../../protos';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { TopLevelCard } from '../../helpers/helpers';
import { title44m } from '../../style/Text/Text';
import { dataAdapter } from './dataAdapter';
import { IMusicLine } from './types';
import { colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../style/constants';
import { MusicCard } from './templates/MusicLine/MusicCard';
import { IRequestState, TTopLevelDivFunction } from '../../../../common/types/common';

function GalleryLine(line: IMusicLine, requestState: IRequestState): Div {
    return new ContainerBlock({
        margins: {
            bottom: 72,
        },
        items: [
            new TextBlock({
                ...title44m,
                text_color: colorWhiteOpacity50,
                text: line.title,
                margins: {
                    left: offsetFromEdgeOfScreen,
                },
            }),
            new GalleryBlock({
                margins: {
                    top: 32,
                },
                paddings: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                item_spacing: 28,
                items: line.items.map((card, index) => MusicCard(card, index, requestState)),
                extensions: [
                    {
                        id: 'centaur-analytics',
                        params: {
                            id: 'main-screen-music-tab-gallery',
                            title: line.title,
                            type: 'gallery',
                        },
                    },
                ],
            }),
        ],
    });
}

function MainScreenMusicTabDiv(
    data: NAlice.NData.ITCentaurMainScreenMusicTabData,
    _: MMRequest,
    requestState: IRequestState,
): Div {
    const {
        lines,
    } = dataAdapter(data);

    return new ContainerBlock({
        items: lines.map(line => GalleryLine(line, requestState)),
    });
}

export const MainScreenMusicTab: TTopLevelDivFunction = (
    data: NAlice.NData.ITCentaurMainScreenMusicTabData,
    mmRequest: MMRequest,
    requestState,
) => {
    return TopLevelCard({
        log_id: 'music_tab',
        states: [
            {
                state_id: 0,
                div: MainScreenMusicTabDiv(data, mmRequest, requestState),
            },
        ],
    }, requestState);
};
