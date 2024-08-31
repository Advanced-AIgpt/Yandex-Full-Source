import type { ISkillCardButton } from '../../types';

export interface ISkillCardBigImage {
    type: 'BigImage',
    image: {
        imageUrl: string,
        title: string,
        description: string,
        button: ISkillCardButton | null,
    },
}
