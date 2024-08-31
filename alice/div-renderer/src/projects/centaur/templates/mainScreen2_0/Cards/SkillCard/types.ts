import { IAbstractCardProps } from '../types';

interface ISkillCardButton {
    text: string;
    payload?: string;
}

export interface ISkillCardProps extends IAbstractCardProps {
    type: 'skill';
    title?: string | null;
    comment?: string | null;
    image?: string | null;
    buttons?: ISkillCardButton[] | null;
    skillName: string;
    skillImage: string;
    skillId: string;
}
