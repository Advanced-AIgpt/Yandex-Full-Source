import { compact, isEmpty } from 'lodash';
import { NAlice } from '../../../../protos';
import { IVideoSearchCard, IVideoSearchGallery, IVideoSearchData } from './types';
import { Avatar } from '../../../../common/helpers/avatar';
import { ExpFlags, hasExperiment } from '../../../../projects/centaur/expFlags';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { Directive, directivesAction } from '../../../../common/actions/index';
import { createSemanticFrameActionTypeSafe } from '../../../../common/actions/server/index';
import { TTypedSemanticFrame } from '../../../../protos/alice/megamind/protos/common/frame';
import { createWebviewClientAction } from '../../actions/client';

function KinoPoiskVideoPlayAction(
    providerItemId: string,
): Directive {
    return createSemanticFrameActionTypeSafe(
        {
            GalleryVideoSelectSemanticFrame: {
                Action: {
                    StringValue: 'play',
                },
                ProviderItemId: {
                    StringValue: providerItemId,
                },
            },
        } as TTypedSemanticFrame,
        'Video',
        'centaur_video_play',
    );
}

function videoSearchCardAdapter(data: NAlice.NTv.ITCarouselItemWrapper, mmRequest: MMRequest): IVideoSearchCard | null {
    const title = data?.VideoItem?.Title || data?.SearchVideoItem?.Title || data?.PersonItem?.Name || data?.CollectionItem?.Title || '';
    const imageUrl = data?.VideoItem?.Poster?.BaseUrl || data?.SearchVideoItem?.Thumbnail?.BaseUrl || data?.PersonItem?.Image?.BaseUrl || data?.CollectionItem?.Images?.[0]?.BaseUrl || '';
    if (!title && !imageUrl) {
        return null;
    }

    const actionUrl = (() => {
        const KinoPoiskItemId = data?.VideoItem?.ProviderItemId;
        if (KinoPoiskItemId) {
            return directivesAction(KinoPoiskVideoPlayAction(KinoPoiskItemId));
        }
        if (data?.SearchVideoItem?.PlayerId == 'youtube') {
            const youTubeUrl = data?.SearchVideoItem?.EmbedUri;
            if (youTubeUrl) {
                return directivesAction(createWebviewClientAction(youTubeUrl));
            }
        }
        return '';
    })();

    if (!hasExperiment(mmRequest, ExpFlags.videoSearchShowAllResults) && !actionUrl) {
        return null;
    }

    const avatar = Avatar.fromUrl(imageUrl)
        ?.setTypeName('664x374', 'video_thumb_fresh')
        .setTypeName('636x636', 'kinopoisk-image')
        .setTypeName('564x318_1', 'vthumb')
        .setTypeName('664x374', 'video_frame')
        .toString() || imageUrl;

    return {
        title,
        imageUrl: avatar,
        actionUrl,
    };
}

function videoSearchGalleryAdapter(data: NAlice.ITTvSearchCarouselWrapper, mmRequest: MMRequest): IVideoSearchGallery | null {
    if (!data.BasicCarousel || !data.BasicCarousel.Items || data.BasicCarousel.Items.length === 0) {
        return null;
    }
    const line = data.BasicCarousel;
    const items = compact(line?.Items?.map(data => videoSearchCardAdapter(data, mmRequest)));

    if (isEmpty(items)) {
        return null;
    }

    return {
        title: line?.Title || '',
        items,
    };
}

export function videoSearchDataAdapter(data: NAlice.ITTvSearchResultData, mmRequest: MMRequest): IVideoSearchData {
    return {
        galleries: compact(data?.Galleries?.map(data => videoSearchGalleryAdapter(data, mmRequest))) || [],
    };
}
