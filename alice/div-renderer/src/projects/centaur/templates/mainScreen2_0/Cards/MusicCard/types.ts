import { IAbstractCardProps } from '../types';

export interface IMusicCardProps extends IAbstractCardProps {
    type: 'music';
    name?: string | null;
    description?: string | null;
    color?: string | null;
    cover?: string | null;
    modified?: string | null;
}
