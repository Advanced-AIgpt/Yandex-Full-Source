import { IRequestState } from '../../../../common/types/common';
import { Div, DivStateBlock, MatchParentSize } from 'divcard2';
import { createRequestState } from '../../../../registries/common';
import { centaurTemplatesClass } from '../../index';
import { DualScreenStates } from './DualScreen';
import { IDualScreenWithOrientationTemplateProps } from './DualScreenWithOrientation/DualScreenWithOrientationTemplate';

export interface IDualScreenProps extends IDualScreenWithOrientationTemplateProps {
}

export default function DualScreenTemplate(inverseOnVertical = false): [Div, IRequestState] {
    const requestState = createRequestState();

    return [
        new DivStateBlock({
            div_id: DualScreenStates.stateId,
            default_state_id: DualScreenStates.stateHorizontal,
            width: new MatchParentSize({ weight: 1 }),
            height: new MatchParentSize({ weight: 1 }),
            transition_animation_selector: 'any_change',
            states: [
                {
                    state_id: DualScreenStates.stateHorizontal,
                    div: centaurTemplatesClass.use<keyof IDualScreenProps, 'dual_screen_with_orientation'>('dual_screen_with_orientation', {
                        orientation: 'horizontal',
                    }, requestState),
                },
                {
                    state_id: DualScreenStates.stateVertical,
                    div: centaurTemplatesClass.use<keyof IDualScreenProps, 'dual_screen_with_orientation' | 'dual_screen_with_orientation_inverse'>(inverseOnVertical ? 'dual_screen_with_orientation_inverse' : 'dual_screen_with_orientation', {
                        orientation: 'vertical',
                    }, requestState),
                },
            ],
        }),
        requestState,
    ];
}
