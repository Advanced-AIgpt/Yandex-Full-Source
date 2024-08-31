import { IDivAction } from 'divcard2';
import { IMusicCard } from '../../types';
import { MUSIC_CARD_COVER_SIZE } from './MusicCardTemplate';
import { IRequestState } from '../../../../../../common/types/common';
import { centaurTemplatesClass } from '../../../../index';
import { musicLoaderElement } from '../../../Music/MusicLoader/MusicLoader';

interface ITemplateCardData {
    cover_url: string;
    title: string;
    border_radius: number;
    subtitle?: string;
    actions?: IDivAction[];
    item_index: number;
}

export function MusicCard(data: IMusicCard, itemIndex: number, requestState: IRequestState) {
    const dataToTemplate: ITemplateCardData = {
        cover_url: data.cover,
        title: data.title,
        border_radius: data.type === 'artist' ? Math.floor(MUSIC_CARD_COVER_SIZE / 2) : 28,
        item_index: itemIndex,
        subtitle: data.subtitle || undefined,
    };

    if (data.action) {
        dataToTemplate.actions = [
            musicLoaderElement({
                title: data.title,
                ImageUrl: data.cover,
                subtitle: data.subtitle,
            }, requestState),
            {
                log_id: 'go_to_music',
                url: data.action,
            },
        ];
    }

    if (dataToTemplate.subtitle) {
        return centaurTemplatesClass.use<'', 'music_card_with_subtitle'>('music_card_with_subtitle', dataToTemplate, requestState);
    }
    return centaurTemplatesClass.use<'', 'music_card'>('music_card', dataToTemplate, requestState);
}
