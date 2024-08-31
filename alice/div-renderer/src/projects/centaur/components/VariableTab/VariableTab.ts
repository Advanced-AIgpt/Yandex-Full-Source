import { Div, DivStateBlock, IDivAnimation, IDivStateBlockState } from 'divcard2';
import { IRequestState } from '../../../../common/types/common';
import { IStatePlace, setStateActionInAllPlaces } from '../../../../common/actions/div';

interface IVariableTabProps {
    items: Div[];
    defaultActive?: number;
    variableName: string;
    stateName: string;
    requestState: IRequestState;
    places?: IStatePlace[];
    options?: Omit<ConstructorParameters<typeof DivStateBlock>[0], 'states' | 'default_state_id' | 'div_id'>;
    animationIn?: IDivAnimation;
    animationOut?: IDivAnimation;
}

export default function VariableTab({
    items,
    stateName,
    variableName,
    requestState,
    defaultActive = 0,
    places = [{ place: ['0'], name: 'top_level' }],
    options,
    animationIn,
    animationOut,
}: IVariableTabProps): Div {
    return new DivStateBlock({
        ...options,
        div_id: stateName,
        default_state_id: `${stateName}_${defaultActive}`,
        states: items.map((el, index): IDivStateBlockState => {
            requestState.variableTriggers.add({
                condition: `@{${variableName} == ${index}.0}`,
                actions: setStateActionInAllPlaces({
                    places,
                    logPrefix: `set ${stateName} ${index} to active`,
                    state: [stateName, `${stateName}_${index}`],
                }),
            });

            return {
                state_id: `${stateName}_${index}`,
                div: el,
                animation_in: animationIn,
                animation_out: animationOut,
            };
        }),
    });
}
