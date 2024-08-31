import {
    ContainerBlock,
    Div,
    ImageBackground,
    FixedSize,
    TextBlock,
    SolidBackground,
    ImageBlock,
    IDivAction,
    MatchParentSize,
} from 'divcard2';
import { title22m, title32m, title36m } from '../../../../style/Text/Text';
import { backgroundToImageWithText, colorWhiteOpacity90, colorBlueTextOpacity50, colorBlueTextOpacity10 } from '../../../../style/constants';
import { IVideoCallLogoutedProps } from './types';
import { BasicTextCard } from '../BasicTextCard';
import { getS3Asset } from '../../../../helpers/assets';
import { basicCardBackground } from '../BasicCard';
import { getCardMainScreenId } from '../helpers';
import { IAbstractCardProps } from '../types';
import { SomeGrid } from '../../../../components/SomeGrid/SomeGrid';
import { MAIN_SCREEN_CORNER_RADIUS } from '../../constants';
import { compact } from 'lodash';

export const DeviceName = 'SmartSpeaker';

export function VideoCallLogoutedCard({ actions, longtap_actions, colIndex, rowIndex }: IVideoCallLogoutedProps): Div {
    const cardData: Parameters<typeof BasicTextCard>[0] = {
        id: getCardMainScreenId({ rowIndex, colIndex }),
        title: 'Настройте',
        background: [
            ...basicCardBackground,
            new ImageBackground({
                image_url: getS3Asset('main-screen/cards/video_call/video_call_setup.png'),
                scale: 'fill',
                preload_required: 1,
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'center',
            }),
            ...backgroundToImageWithText,
        ],
        items: [
            new TextBlock({
                ...title32m,
                max_lines: 3,
                text_color: colorWhiteOpacity90,
                text: 'Видео звонки через телеграм',
            }),
        ],
        actions,
        longtap_actions,
    };

    return BasicTextCard(cardData);
}

export interface IFavoriteContactItem {
    title: string;
    userId: string;
    actions?: IDivAction[];
    longtap_actions?: IDivAction[];
}
const VideoCallLoginedCardFavorite = ({ title, userId, longtap_actions, actions }: IFavoriteContactItem): Div => {
    const shortName = (() => {
        const [firstName = '', secondName = ''] = title.split(' ');

        return compact([
            firstName.slice(0, 1).toUpperCase(),
            secondName.slice(0, 1).toUpperCase(),
        ]).join('');
    })();

    return new ContainerBlock({
        orientation: 'vertical',
        width: new FixedSize({ value: 104 }),
        actions,
        longtap_actions,
        items: [
            new ContainerBlock({
                orientation: 'overlap',
                height: new FixedSize({ value: 86 }),
                width: new FixedSize({ value: 86 }),
                margins: { left: 6, right: 6 },
                items: [
                    new ContainerBlock({
                        height: new FixedSize({ value: 86 }),
                        width: new FixedSize({ value: 86 }),
                        border: {
                            corner_radius: 1000,
                        },
                        background: [new SolidBackground({ color: colorBlueTextOpacity10 })],
                        items: [
                            new TextBlock({
                                ...title36m,
                                text: shortName,
                                height: new MatchParentSize(),
                                width: new MatchParentSize(),
                                text_alignment_horizontal: 'center',
                                text_alignment_vertical: 'center',
                            }),
                        ],
                    }),
                    new ImageBlock({
                        height: new FixedSize({ value: 86 }),
                        width: new FixedSize({ value: 86 }),
                        border: {
                            corner_radius: 1000,
                        },
                        image_url: ' ',
                        alignment_horizontal: 'center',
                        extensions: [
                            {
                                id: 'telegram-avatar',
                                params: {
                                    user_id: userId,
                                },
                            },
                        ],
                    }),
                ],
            }),
            new TextBlock({
                ...title22m,
                text: title,
                width: new MatchParentSize(),
                height: new FixedSize({ value: 28 }),
                text_alignment_horizontal: 'center',
                alignment_horizontal: 'center',
                text_color: colorBlueTextOpacity50,
                max_lines: 1,
                margins: { top: 6 },
            }),
        ],
    });
};

export interface IVideoCallLoginedProps extends IAbstractCardProps {
    type: 'video_call_logined';
    contactsUploaded?: boolean | null;
    favorites: IFavoriteContactItem[];
}
export const VideoCallLoginedCard = ({
    favorites,
    rowIndex,
    colIndex,
    longtap_actions,
    actions,
}: IVideoCallLoginedProps) => {
    const items = favorites.map(item => VideoCallLoginedCardFavorite({ ...item, longtap_actions }));

    return SomeGrid({
        id: getCardMainScreenId({ rowIndex, colIndex }),
        height: new MatchParentSize(),
        background: [...basicCardBackground],
        border: {
            corner_radius: MAIN_SCREEN_CORNER_RADIUS,
        },
        paddings: {
            top: 14,
            left: 24,
            right: 24,
            bottom: 14,
        },
        longtap_actions,
        actions,
        items,
        columnCount: 3,
    });
};
