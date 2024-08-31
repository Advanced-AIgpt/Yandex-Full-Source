import { setStateAction } from '../../../../common/actions/div';
import { centaurTemplatesClass } from '../../index';
import { IDualScreenProps } from './DualScreenTemplate';
import { IRequestState } from '../../../../common/types/common';

export enum DualScreenStates {
    stateId = 'dual_screen_orientation',
    stateHorizontal = 'horizontal',
    stateVertical = 'vertical',
}

const changeOrientationTriggers = [
    {
        condition: '@{isPortrait}',
        actions: [
            {
                log_id: 'orientation_horizontal',
                url: setStateAction([
                    '0',
                    DualScreenStates.stateId,
                    DualScreenStates.stateVertical,
                ]),
            },
        ],
    },
    {
        condition: '@{!isPortrait}',
        actions: [
            {
                log_id: 'orientation_vertical',
                url: setStateAction([
                    '0',
                    DualScreenStates.stateId,
                    DualScreenStates.stateHorizontal,
                ]),
            },
        ],
    },
];

export function DualScreen(
    {
        inverseOnVertical = false,
        requestState,
        firstDiv,
        secondDiv,
        mainColor,
        mainColor1,
    }: IDualScreenProps &
        {
            requestState: IRequestState;
            inverseOnVertical?: boolean;
        },
) {
    requestState.variableTriggers.add(changeOrientationTriggers);

    return centaurTemplatesClass.use<'', 'dual_screen' | 'dual_screen_inverse'>(inverseOnVertical ? 'dual_screen_inverse' : 'dual_screen', {
        firstDiv,
        secondDiv,
        mainColor,
        mainColor1,
    }, requestState);
}
