import { ContainerBlock, Div, FixedSize, ImageBackground, ImageBlock, TextBlock, WrapContentSize } from 'divcard2';
import { IWeatherCardProps } from './types';
import { text28m, title90m } from '../../../../style/Text/Text';
import { colorWhite, colorWhiteOpacity50 } from '../../../../style/constants';
import { BasicTextCard } from '../BasicTextCard';
import BasicErrorCard from '../BasicErrorCard';
import { logger } from '../../../../../../common/logger';
import { getTemperatureString } from '../../../../helpers/weather/weather';
import { getCardMainScreenId } from '../helpers';

export default function WeatherCard({
    city,
    comment,
    image,
    temperature,
    bgImage,
    actions,
    longtap_actions,
    rowIndex,
    colIndex,
}: Partial<IWeatherCardProps>): Div {
    if (!city || !comment || !image || typeof temperature === 'undefined' || temperature === null) {
        logger.error('The weather card should have a city, a comment, a picture and a temperature, but some of this was not transmitted');

        return BasicErrorCard({
            rowIndex,
            colIndex,
            title: 'Погода',
            description: 'Ошибка сети. Не могу показать прозноз. Проверьте интернет',
            actions,
            longtap_actions,
        });
    }

    const cardData: Parameters<typeof BasicTextCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        title: city,
        items: [
            new ContainerBlock({
                orientation: 'horizontal',
                content_alignment_vertical: 'center',
                content_alignment_horizontal: 'left',
                items: [
                    new TextBlock({
                        ...title90m,
                        line_height: 90,
                        text_color: colorWhite,
                        width: new WrapContentSize(),
                        text: getTemperatureString(temperature),
                    }),
                    new ImageBlock({
                        height: new FixedSize({ value: 100 }),
                        width: new FixedSize({ value: 100 }),
                        margins: {
                            left: 10,
                        },
                        image_url: image,
                    }),
                ],
            }),
            new TextBlock({
                ...text28m,
                text_color: colorWhiteOpacity50,
                text: comment,
            }),
        ],
        actions,
        longtap_actions,
    };

    if (bgImage) {
        cardData.background = [
            new ImageBackground({
                image_url: bgImage,
                scale: 'fill',
                preload_required: 1,
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
            }),
        ];
    }

    return BasicTextCard(cardData);
}
