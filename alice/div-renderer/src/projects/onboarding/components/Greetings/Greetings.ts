import { Div } from 'divcard2';
import { NAlice } from '../../../../protos';
import { IRequestState } from '../../../../common/types/common';
import { onboardingTemplateHelper } from '../../';

interface GreetingsProps {
    data: NAlice.NData.ITGreetingsCardData;
}

export default function Greetings(
    props: GreetingsProps,
    requestState: IRequestState,
): Div[] {
    const buttons = props.data.Buttons;

    if (!buttons) {
        throw new Error('Onboarding no buttons');
    }

    requestState.localTemplates.add('onboardingGreeting');

    return buttons.map((button, idx) => onboardingTemplateHelper.onboardingGreeting({
        text: button.Title || '',
        action: button.ActionId ? {
            url: `@@mm_deeplink#${button.ActionId}`,
            log_id: `button/${idx}`,
        } : undefined,
        margins: {
            bottom: 6,
            left: idx > 1 ? 6 : undefined,
        },
    }));
}
