import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { IMusicCard, IMusicLine, IMusicTabData } from './types';
import { updatedAt } from '../../helpers/helpers';
import { directivesAction } from '../../../../common/actions';
import { createServerActionBasedOnTSF } from '../../../../common/actions/server';

function getSubTitle(data: NAlice.NData.ITCentaurMainScreenGalleryMusicCardData): string | undefined {
    switch (data.Type) {
        case 'auto_playlist': {
            return updatedAt(new Date(data.Modified?.value ?? 0));
        }
        case 'album': {
            return data.Artists?.map(({ Name }) => Name).join(', ') ?? undefined;
        }
        case 'artist': {
            return data.Genres?.join(', ') ?? undefined;
        }
        default: {
            return undefined;
        }
    }
}

function musicCardDataAdapter(data: NAlice.NData.ITCentaurMainScreenGalleryMusicCardData): IMusicCard | null {
    if (!data.Id || !data.Title) {
        return null;
    }

    const serverAction = createServerActionBasedOnTSF({
        binaryTsf: data.TypedAction?.value,
        productScenario: 'CentaurMainScreen',
        purpose: 'music_play_from_centaur_main_screen',
    });

    return {
        id: data.Id,
        action: serverAction ? (directivesAction(serverAction)) : data.Action ?? undefined,
        title: data.Title,
        cover: data.ImageUrl || ' ',
        subtitle: getSubTitle(data),
        type: data.Type || 'notype',
    };
}

function musicCardLineDataAdapter(data: NAlice.NData.ITHorizontalMusicBlockData): IMusicLine {
    return {
        title: data.Title || ' ',
        type: data.Type || null,
        items: compact(data.CentaurMainScreenGalleryMusicCardData?.map(musicCardDataAdapter)) || [],
    };
}

export function dataAdapter(data: NAlice.NData.ITCentaurMainScreenMusicTabData): IMusicTabData {
    return {
        lines: data.HorizontalMusicBlockData?.map(musicCardLineDataAdapter) || [],
    };
}
