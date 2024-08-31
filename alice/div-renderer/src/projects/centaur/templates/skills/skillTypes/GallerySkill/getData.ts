import { NAlice } from '../../../../../../protos';
import {
    ISkillCardGallery,
} from './types';
type ITImageItem = NAlice.NData.TDialogovoSkillCardData.TSkillResponse.ITImageItem;

function getImageData(image: ITImageItem) {
    return {
        imageUrl: image.ImageUrl || '',
        description: image.Description || '',
        title: image.Title || '',
    };
}

export function getGalleryData(card: NAlice.NData.TDialogovoSkillCardData.ITSkillResponse): ISkillCardGallery | null {
    if (card.ImageGalleryResponse) {
        return {
            type: 'Gallery',
            images: card.ImageGalleryResponse.ImageItems?.map(getImageData) || [],
        };
    }

    return null;
}
