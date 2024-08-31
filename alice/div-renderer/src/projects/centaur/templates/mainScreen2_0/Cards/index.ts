import { Div } from 'divcard2';
import { compact } from 'lodash';
import EmptyCard from './EmptyCard/EmptyCard';
import MusicCard from './MusicCard/MusicCard';
import InfoCard from './InfoCard/InfoCard';
import NewsCard from './NewsCard/NewsCard';
import WeatherCard from './WeatherCard/WeatherCard';
import TrafficCard from './TrafficCard/TrafficCard';
import { VideoCallLoginedCard, VideoCallLogoutedCard } from './VideoCallCard/VideoCallCard';
import { NAlice } from '../../../../../protos';
import { CardProtoDataAdapter, ICardDataOptions } from '../CardProtoDataAdapter';
import YouTubeCard from './YouTubeCard/YouTubeCard';
import ErrorCard from './ErrorCard/ErrorCard';
import VacantCard from './EmptyCard/VacantCard';
import { logger } from '../../../../../common/logger';
import { closeLayerAction } from '../../../actions/client';
import { Layer } from '../../../common/layers';
import { CrutchLoader } from '../../../components/Loader/Loader';
import { IRequestState } from '../../../../../common/types/common';
import SkillsCard from './SkillCard/SkillsCard';

type ITCentaurWidgetCardData = NAlice.NData.ITCentaurWidgetCardData;

export interface ICardInfo {
    name: string;
    div: Div;
}

export function getCard(
    cardData: ITCentaurWidgetCardData,
    requestState: IRequestState,
    options: ICardDataOptions,
): ICardInfo {
    const data = CardProtoDataAdapter(cardData, requestState, options);
    const type = data.type;

    if (data.actions?.length && !data.preventDefaultLoader) {
        data.actions = compact([
            options?.isChoice ? closeLayerAction('close_widgets_layer', Layer.CONTENT, false) : CrutchLoader(data.layer),
            ...data.actions,
        ]);
    }

    data.requestState = requestState;

    switch (data.type) {
        case 'weather':
            return {
                name: 'Погода',
                div: WeatherCard(data),
            };
        case 'music':
            return {
                name: 'Музыка',
                div: MusicCard(data),
            };
        case 'info':
            return {
                name: 'Уведомления',
                div: InfoCard(data),
            };
        case 'news':
            return {
                name: 'Новости',
                div: NewsCard(data),
            };
        case 'youtube':
            return {
                name: 'Быстрое включение',
                div: YouTubeCard(data),
            };
        case 'traffic':
            return {
                name: 'Пробки',
                div: TrafficCard(data),
            };
        case 'video_call_logouted':
            return {
                name: 'Видезвонки',
                div: VideoCallLogoutedCard(data),
            };
        case 'video_call_logined':
            return {
                name: 'Видезвонки',
                div: VideoCallLoginedCard(data),
            };
        case 'empty': {
            const Card = options?.isChoice ? VacantCard : EmptyCard;
            return {
                name: 'Убрать виджет',
                div: Card(data),
            };
        }
        case 'error':
            return {
                name: ' ',
                div: ErrorCard({ colIndex: options.colIndex, rowIndex: options.rowIndex }),
            };
        case 'skill':
            return {
                name: data.skillName,
                div: SkillsCard(data),
            };
        default: {
            // Хак typescript для определения забытых обработчиков
            const unknownData: never = data;
            logger.error(`Unknown data with type ${type}`, unknownData);
            return {
                name: ' ',
                div: ErrorCard({ colIndex: options.colIndex, rowIndex: options.rowIndex }),
            };
        }
    }
}
