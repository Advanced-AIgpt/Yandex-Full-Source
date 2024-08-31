import { ContainerBlock, Div, DivStateBlock } from 'divcard2';
import { setStateActionInAllPlaces, setVariable } from '../../../../common/actions/div';
import { IButtonsLikeRadioProps } from './types';

export function getButtonListVariableId(buttonListId: string) {
    return `buttonList_${buttonListId}`;
}

export default function ButtonsLikeRadio({
    buttons,
    buttonListId,
    startActive = 0,
    requestState,
    places = [{ name: 'top_level', place: ['0'] }],
    options,
}: IButtonsLikeRadioProps): Div[] {
    requestState.variables.add({
        type: 'number',
        name: getButtonListVariableId(buttonListId),
        value: startActive,
    });

    return buttons.map((el, index) => {
        const stateId = `${buttonListId}_button_${index}`;
        const activeId = `${buttonListId}_button_${index}_active`;
        const inactiveId = `${buttonListId}_button_${index}_inactive`;

        requestState.variableTriggers.add({
            condition: `@{${getButtonListVariableId(buttonListId)} == ${index}.0}`,
            actions: setStateActionInAllPlaces({
                places,
                logPrefix: `Set button ${index} of button list ${buttonListId} to active`,
                state: [
                    stateId,
                    activeId,
                ],
            }),
        });
        requestState.variableTriggers.add({
            condition: `@{${getButtonListVariableId(buttonListId)} != ${index}.0}`,
            actions: setStateActionInAllPlaces({
                places,
                logPrefix: `Set button ${index} of button list ${buttonListId} to inactive`,
                state: [
                    stateId,
                    inactiveId,
                ],
            }),
        });

        return new ContainerBlock({
            items: [
                new DivStateBlock({
                    ...options,
                    div_id: stateId,
                    default_state_id: startActive === index ? activeId : inactiveId,
                    states: [
                        {
                            state_id: activeId,
                            div: el.activeDiv([]),
                        },
                        {
                            state_id: inactiveId,
                            div: el.inactiveDiv([
                                {
                                    log_id: 'set active variable',
                                    url: setVariable(getButtonListVariableId(buttonListId), index),
                                },
                            ]),
                        },
                    ],
                }),
            ],
        });
    });
}
