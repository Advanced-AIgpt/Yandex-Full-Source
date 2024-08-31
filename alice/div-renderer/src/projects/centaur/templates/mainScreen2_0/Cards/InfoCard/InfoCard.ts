import { Div, ImageBackground, SolidBackground, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import { text28m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50, colorWhiteOpacity90 } from '../../../../style/constants';
import { IInfoCardProps } from './types';
import { BasicTextCard } from '../BasicTextCard';
import BasicErrorCard from '../BasicErrorCard';
import { logger } from '../../../../../../common/logger';
import { getCardMainScreenId } from '../helpers';

export default function InfoCard({
    color,
    title,
    description,
    subcomment,
    image_background,
    actions,
    longtap_actions,
    rowIndex,
    colIndex,
}: Partial<IInfoCardProps>): Div {
    if (!title) {
        logger.error(new Error('The information card should have a title, but it was not transmitted.'));

        return BasicErrorCard({
            colIndex,
            rowIndex,
            title: 'Навык',
            description: 'Произошла ошибка. Проверьте интернет или обновите экран!',
            actions,
            longtap_actions,
        });
    }

    const cardData: Parameters<typeof BasicTextCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        title: title || ' ',
        background: compact([
            color && new SolidBackground({ color }),
            image_background && new ImageBackground({
                image_url: image_background,
                content_alignment_horizontal: 'right',
                scale: 'fill',
            }),
        ]),
        items: compact([
            description && new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity90,
                text: description,
            }),
            subcomment && new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity50,
                text: subcomment,
                margins: {
                    top: 6,
                },
            }),
        ]),
        actions,
        longtap_actions,
    };

    return BasicTextCard(cardData);
}
