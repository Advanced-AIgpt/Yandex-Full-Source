import {
    ContainerBlock,
    FixedSize,
    GalleryBlock,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { updatedAt } from '../../helpers/helpers';
import { colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../style/constants';
import { text28m, title32m, title44m } from '../../style/Text/Text';
import { musicLoaderElement } from '../FullMusic/musicLoader';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags, hasExperiment } from '../../expFlags';
import { IRequestState } from '../../../../common/types/common';

type MusicCaption = {
    title: string;
    subtitle?: string;
    imageBorder: ImageBlock['border']
};
const musicCaption = ({
    Title,
    Type,
    Modified,
    Artists,
    Genres,
}: NAlice.NData.ITCentaurMainScreenGalleryMusicCardData): MusicCaption => {
    const result: MusicCaption = {
        title: Title ?? '',
        imageBorder: { corner_radius: 28 },
    };
    switch (Type) {
        case 'auto_playlist': {
            result.subtitle = updatedAt(new Date(Modified?.value ?? 0));
            break;
        }
        case 'album': {
            result.subtitle = Artists?.map(({ Name }) => Name).join(', ') ?? '';
            break;
        }
        case 'artist': {
            result.subtitle = Genres?.join(', ') ?? '';
            result.imageBorder = {
                corner_radius: Math.floor(CARD_SIZE / 2),
            };
            break;
        }
    }

    return result;
};

const CARD_SIZE = 275;

const renderMusicCard = (
    data: NAlice.NData.ITCentaurMainScreenGalleryMusicCardData,
    requestState: IRequestState,
    first: boolean = false,
    last: boolean = false,
    musicLoader = false,
    idx = 0,
) => {
    const {
        Action,
        ImageUrl,
        Id,
    } = data;
    const { title, subtitle, imageBorder } = musicCaption(data);

    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        margins: {
            left: first ? offsetFromEdgeOfScreen : 0,
            right: last ? offsetFromEdgeOfScreen : 0,
        },
        orientation: 'vertical',
        items: compact([
            new ImageBlock({
                width: new FixedSize({ value: CARD_SIZE }),
                height: new FixedSize({ value: CARD_SIZE }),
                image_url: ImageUrl ?? '',
                actions: compact([
                    musicLoader && musicLoaderElement({
                        Id,
                        subtitle,
                        ImageUrl,
                        title,
                        requestState,
                    }),
                    {
                        log_id: 'main_screen.music.click.' + Id,
                        url: Action ?? '',
                    },
                ]),
                preload_required: 1,
                border: imageBorder,
                extensions: [
                    {
                        id: 'centaur-analytics',
                        params: {
                            id: 'main-screen-music-tab-gallery-element',
                            title,
                            type: 'music',
                            pos: idx,
                        },
                    },
                ],
            }),
            new TextBlock({
                ...title32m,
                width: new FixedSize({ value: CARD_SIZE }),
                height: new WrapContentSize(),
                margins: {
                    top: 20,
                },
                text: title,
                max_lines: 2,
            }),
            subtitle &&
            new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity50,
                width: new FixedSize({ value: CARD_SIZE }),
                height: new WrapContentSize(),
                text: subtitle,
                max_lines: 1,
            }),
        ]),
    });
};

export const renderMainScreenMusicCard = (data: NAlice.NData.ITCentaurMainScreenGalleryMusicCardData, _:MMRequest, requestState: IRequestState) => {
    return new TemplateCard(new Templates({}), {
        log_id: 'main_screen.music',
        states: [
            {
                state_id: 0,
                div: renderMusicCard(data, requestState),
            },
        ],
    });
};

const renderHorizontalBlock = (
    { Title, CentaurMainScreenGalleryMusicCardData }: NAlice.NData.ITHorizontalMusicBlockData,
    requestState: IRequestState,
    first: boolean,
    musicLoader = false,
) => {
    const length = CentaurMainScreenGalleryMusicCardData?.length;
    const blockText = Title ?? '';

    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        margins: {
            top: first ? 0 : 72,
        },
        items: [
            new TextBlock({
                ...title44m,
                text_color: colorWhiteOpacity50,
                text: blockText,
                margins: {
                    left: offsetFromEdgeOfScreen,
                },
            }),
            new GalleryBlock({
                margins: {
                    top: 32,
                },
                item_spacing: 28,
                items: CentaurMainScreenGalleryMusicCardData
                    ?.map((el, idx) => renderMusicCard(el, requestState, idx === 0, idx + 1 === length, musicLoader, idx)) ?? [],
                extensions: [
                    {
                        id: 'centaur-analytics',
                        params: {
                            id: 'main-screen-music-tab-gallery',
                            title: blockText,
                            type: 'gallery',
                        },
                    },
                ],
            }),
        ],
    });
};

/**
 * Рендер таба музыки на главном экране
 * @param data
 * @param mmRequest
 * @param requestState
 * @deprecated
 */
export const renderMainMusicTab = (data: NAlice.NData.ITCentaurMainScreenMusicTabData, mmRequest: MMRequest, requestState: IRequestState) => {
    const musicLoader = hasExperiment(mmRequest, ExpFlags.musicPerformanceOpen);

    return new TemplateCard(new Templates({}), {
        log_id: data.Id ?? '',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new WrapContentSize(),
                    items:
                        data.HorizontalMusicBlockData
                            ?.map((item, idx) => renderHorizontalBlock(item, requestState, idx === 0, musicLoader)) ?? [],
                }),
            },
        ],
    });
};
