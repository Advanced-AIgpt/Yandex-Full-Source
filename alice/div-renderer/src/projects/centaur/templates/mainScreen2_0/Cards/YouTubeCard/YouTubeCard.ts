import { FixedSize, ImageBlock, MatchParentSize, TextBlock } from 'divcard2';
import { BasicCard } from '../BasicCard';
import { title32m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../../style/constants';
import EmptyDiv from '../../../../components/EmptyDiv';
import { getS3Asset } from '../../../../helpers/assets';
import { IYouTubeProps } from './types';
import { getCardMainScreenId } from '../helpers';

export default function YouTubeCard({ actions, longtap_actions, rowIndex, colIndex } : IYouTubeProps) {
    const cardData: Parameters<typeof BasicCard>[0] = {
        id: getCardMainScreenId({ rowIndex: rowIndex || 0, colIndex: colIndex || 0 }),
        items: [
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                text: 'Youtube',
            }),
            new EmptyDiv({
                height: new MatchParentSize({ weight: 31 }),
            }),
            new ImageBlock({
                alignment_horizontal: 'center',
                alignment_vertical: 'center',
                height: new FixedSize({ value: 112 }),
                width: new FixedSize({ value: 161 }),
                image_url: getS3Asset('main-screen/cards/youtube/logo.png'),
                margins: {
                    top: 31,
                },
            }),
            new EmptyDiv({
                height: new MatchParentSize({ weight: 56 }),
            }),
        ],
        actions,
        longtap_actions,
    };

    return BasicCard(cardData);
}
