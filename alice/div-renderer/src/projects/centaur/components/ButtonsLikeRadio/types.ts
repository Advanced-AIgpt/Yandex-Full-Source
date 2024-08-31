import { Div, IDivAction } from 'divcard2';
import { IRequestState } from '../../../../common/types/common';
import { IStatePlace } from '../../../../common/actions/div';
import { DivStateBlockProps } from '../../helpers/types';

type createDivWithAction = (actions?: IDivAction[]) => Div;

interface IButtonLikeRadioProps {
    activeDiv: createDivWithAction;
    inactiveDiv: createDivWithAction;
}

export interface IButtonsLikeRadioProps {
    buttons: IButtonLikeRadioProps[];
    requestState: IRequestState;
    buttonListId: string;
    startActive?: number;
    places?: IStatePlace[];
    options?: Omit<DivStateBlockProps, 'states' | 'default_state_id' | 'div_id'>;
}
