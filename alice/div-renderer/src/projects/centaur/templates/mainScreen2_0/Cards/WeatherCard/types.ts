import { IAbstractCardProps } from '../types';

export interface IWeatherCardProps extends IAbstractCardProps {
    type: 'weather';
    city?: string | null;
    temperature?: number | null;
    image?: string | null;
    comment?: string | null;
    bgImage?: string | null;
}
