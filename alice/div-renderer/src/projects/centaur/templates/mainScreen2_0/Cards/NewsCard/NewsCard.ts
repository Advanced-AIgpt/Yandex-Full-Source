import { Div, ImageBackground, TextBlock } from 'divcard2';
import { text28m } from '../../../../style/Text/Text';
import { backgroundToImageWithText, colorWhiteOpacity90 } from '../../../../style/constants';
import { INewsCardProps } from './types';
import { BasicTextCard } from '../BasicTextCard';
import { logger } from '../../../../../../common/logger';
import BasicErrorCard from '../BasicErrorCard';
import { Avatar } from '../../../../../../common/helpers/avatar';
import { basicCardBackground } from '../BasicCard';
import { getCardMainScreenId } from '../helpers';

export default function NewsCard({
    title,
    content,
    image,
    actions,
    longtap_actions,
    colIndex,
    rowIndex,
}: INewsCardProps): Div {
    if (!title || !content) {
        logger.error(new Error('The news card should have a title and description, but some of this was not transmitted.'));

        return BasicErrorCard({
            colIndex,
            rowIndex,
            title: 'Новости',
            description: 'Ошибка сети. Не могу показать новости. Проверьте интернет.',
            actions,
            longtap_actions,
        });
    }

    const cardData: Parameters<typeof BasicTextCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        title,
        items: [
            new TextBlock({
                ...text28m,
                max_lines: 5,
                text_color: colorWhiteOpacity90,
                text: content,
            }),
        ],
        actions,
        longtap_actions,
    };

    if (image) {
        const avatar = Avatar.fromUrl(image);

        cardData.background = [
            ...basicCardBackground,
            new ImageBackground({
                image_url: avatar && avatar.namespace === 'ynews' ? avatar.setTypeName('380x214').toString() : image,
                scale: 'fill',
                preload_required: 1,
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
            }),
            ...backgroundToImageWithText,
        ];
    }

    return BasicTextCard(cardData);
}
