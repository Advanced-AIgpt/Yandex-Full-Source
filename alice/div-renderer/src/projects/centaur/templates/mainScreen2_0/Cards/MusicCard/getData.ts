import { IMusicCardProps } from './types';
import { colorDarkGrey } from '../../../../style/constants';
import { ICardDataAdapter } from '../types';

const playlists: readonly [string, string][] = [
    ['Плейлист дня', '#31AD54'],
    ['Мне нравится', '#E53866'],
    ['Тайник', '#186FD3'],
    ['Подкасты', '#036292'],
    ['Дежавю', '#A019D1'],
    ['Премьера', '#D1673E'],
    ['Плейлист с Алисой', '#2D0B87'],
    ['Мой 2021', '#C20146'],
    ['Плейлист семьи', '#FE8853'],
];

export const getMusicCardData: ICardDataAdapter<IMusicCardProps> = function getMusicCardData(card, requestState) {
    if (typeof card.MusicCardData !== 'undefined' && card.MusicCardData !== null) {
        const data = card.MusicCardData;

        const knownPlaylist = playlists.find(el => el[0] === data.Name);

        const color = knownPlaylist ? knownPlaylist[1] : colorDarkGrey;

        return {
            type: 'music',
            name: data.Name,
            modified: data.Modified?.value,
            color,
            cover: data.Cover,
            actions: (card.Action && [
                {
                    log_id: 'music_card_action',
                    url: card.Action,
                },
            ]) || undefined,
            preventDefaultLoader: true,
            requestState,
        };
    }
    return null;
};
