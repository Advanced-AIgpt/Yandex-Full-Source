import { SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { TextBlockProps } from '../../helpers/types';
import { title22m } from '../../style/Text/Text';

export const yaMapsRatingColorMap = {
    best: '#5FB753',
    good: '#97C752',
    notGood: '#95A356',
    bad: '#858561',
};

export const getYaMapsRatingBackgroundColor = (rating: number) => {
    if (rating >= 4) {
        return yaMapsRatingColorMap.best;
    }

    if (rating >= 3 && rating < 4) {
        return yaMapsRatingColorMap.good;
    }

    if (rating >= 1 && rating < 3) {
        return yaMapsRatingColorMap.notGood;
    }

    return yaMapsRatingColorMap.bad;
};

export const formatRatingValue = (rating: number) => {
    const isInteger = rating % 1 === 0;

    return isInteger ? [rating, 0].join(',') : String(rating).replace('.', ',');
};

interface YaMapsRatingProps extends TextBlockProps {
    rating: number;
}
export const YaMapsRating = ({ rating, ...props }: YaMapsRatingProps) => {
    return new TextBlock({
        ...props,
        ...title22m,
        text: formatRatingValue(rating),
        width: new WrapContentSize(),
        background: [new SolidBackground({ color: getYaMapsRatingBackgroundColor(rating) })],
        paddings: {
            top: 4,
            left: 12,
            right: 12,
            bottom: 4,
        },
        border: {
            corner_radius: 12,
        },
    });
};
