import { IAbstractCardProps } from '../types';

export interface ITrafficForecast {
    time: number | null;
    trafficValue?: number | null;
}

export interface ITrafficCardProps extends IAbstractCardProps {
    type: 'traffic';
    city?: string | null;
    text?: string | null;
    trafficValue?: number | null;
    forecasts?: ITrafficForecast[];
}
