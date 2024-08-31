import { IAbstractCardProps } from '../types';

export interface INewsCardProps extends IAbstractCardProps {
    type: 'news';
    title?: string | null;
    content?: string | null;
    image?: string | null;
}
