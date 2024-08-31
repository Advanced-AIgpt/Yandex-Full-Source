import { IColorSet } from '../../style/colorSet';
import { IDivAction } from 'divcard2';

export interface ISuggest {
    colorSet: IColorSet;
    text: string;
    actions: IDivAction[];
}
