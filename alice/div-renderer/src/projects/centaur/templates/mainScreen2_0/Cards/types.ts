import { IDivAction } from 'divcard2';
import { IInfoCardProps } from './InfoCard/types';
import { IMusicCardProps } from './MusicCard/types';
import { IWeatherCardProps } from './WeatherCard/types';
import { IYouTubeProps } from './YouTubeCard/types';
import { INewsCardProps } from './NewsCard/types';
import { IEmptyCardProps } from './EmptyCard/types';
import { IVideoCallLogoutedProps } from './VideoCallCard/types';
import { IErrorCardProps } from './ErrorCard/types';
import { NAlice } from '../../../../../protos';
import { EnumLayer } from '../../../actions/client';
import { ITrafficCardProps } from './TrafficCard/types';
import { IRequestState } from '../../../../../common/types/common';
import { ISkillCardProps } from './SkillCard/types';
import { IVideoCallLoginedProps } from './VideoCallCard/VideoCallCard';
import { ICardDataOptions } from '../CardProtoDataAdapter';

type ITCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.ITCardData;

export type CardInfo =
    IInfoCardProps |
    IMusicCardProps |
    INewsCardProps |
    IWeatherCardProps |
    IYouTubeProps |
    IVideoCallLoginedProps |
    IVideoCallLogoutedProps |
    IEmptyCardProps |
    IErrorCardProps |
    ITrafficCardProps |
    ISkillCardProps;

type CardRow = CardInfo;
type CardColumn = CardRow[];
export type CardsData = CardColumn[];

export type ICardDataAdapter<T extends IAbstractCardProps> = (data: Readonly<ITCardData>, requestState: IRequestState, options?: ICardDataOptions) => T | null;

export interface IAbstractCardProps {
    type: CardInfo['type'];
    actions?: IDivAction[];
    longtap_actions?: IDivAction[];
    layer?: EnumLayer;
    colIndex?: number;
    rowIndex?: number;
    requestState: IRequestState;
    preventDefaultLoader?: boolean;
}
