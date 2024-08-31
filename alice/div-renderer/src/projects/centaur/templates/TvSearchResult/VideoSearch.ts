import {
    ContainerBlock,
    Div,
    FixedSize,
    GalleryBlock,
    ImageBlock,
    SolidBackground,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import { title32m, title44m } from '../../style/Text/Text';
import { colorWhiteOpacity10, colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../style/constants';
import { compact } from 'lodash';
import { videoSearchDataAdapter } from './dataAdapter';
import { IVideoSearchCard, IVideoSearchGallery, IVideoSearchData } from './types';
import { MMRequest } from '../../../../common/helpers/MMRequest';

const VIDEO_CARD_SIZE = 275;

function VideoSearchCard(item: IVideoSearchCard): Div {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        orientation: 'vertical',
        items: compact([
            new ImageBlock({
                width: new FixedSize({ value: VIDEO_CARD_SIZE }),
                height: new FixedSize({ value: VIDEO_CARD_SIZE }),
                background: [
                    new SolidBackground({ color: colorWhiteOpacity10 }),
                ],
                image_url: item.imageUrl,
                preload_required: 1,
                border: {
                    corner_radius: 10,
                },
            }),
            new TextBlock({
                ...title32m,
                width: new FixedSize({ value: VIDEO_CARD_SIZE }),
                height: new WrapContentSize(),
                paddings: {
                    top: 20,
                },
                text: item.title,
                max_lines: 2,
            }),
        ]),
        actions: [
            {
                log_id: 'centaur_video_play_action',
                url: item.actionUrl,
            },
        ],
    });
}

function VideoSearchGallery(gallery: IVideoSearchGallery): Div {
    return new ContainerBlock({
        margins: {
            bottom: 72,
        },
        items: [
            new TextBlock({
                ...title44m,
                text_color: colorWhiteOpacity50,
                text: gallery.title,
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
                items: (gallery.items || []).map(item => VideoSearchCard(item)),
                extensions: [
                    {
                        id: 'centaur-analytics',
                        params: {
                            id: 'video-search-gallery',
                            title: gallery.title,
                            type: 'gallery',
                        },
                    },
                ],
            }),
        ],
    });
}

function VideoSearchDiv(data: IVideoSearchData) {
    return new ContainerBlock({
        items: data.galleries.map(gallery => VideoSearchGallery(gallery)),
    });
}

export default function VideoSearch(data: NAlice.ITTvSearchResultData, mmRequest: MMRequest) {
    const params = videoSearchDataAdapter(data, mmRequest);
    return new TemplateCard(new Templates({}), {
        log_id: 'video_search',
        states: [
            {
                state_id: 0,
                div: VideoSearchDiv(params),
            },
        ],
    });
}
