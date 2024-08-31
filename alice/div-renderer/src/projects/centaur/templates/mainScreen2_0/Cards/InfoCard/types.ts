import { IAbstractCardProps } from '../types';

export interface IInfoCardProps extends IAbstractCardProps {
    type: 'info';
    color?: string | null;
    image_background?: string | null;
    title?: string | null;
    description?: string | null;
    subcomment?: string | null;
    icon?: string | null;
}
