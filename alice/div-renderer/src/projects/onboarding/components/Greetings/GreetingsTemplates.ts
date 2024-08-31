import {
    TextBlock,
    WrapContentSize,
    Template,
} from 'divcard2';
import { greetingsPalette } from './GreetingsColors';

export default function() {
    return {
        onboardingGreeting: new TextBlock({
            text: new Template('text'),
            text_color: greetingsPalette.ns.text.primary,
            font_size: 14,
            font_weight: 'medium',
            line_height: 16,
            width: new WrapContentSize(),
            paddings: {
                left: 18,
                right: 18,
                top: 14,
                bottom: 14,
            },
            border: {
                corners_radius: {
                    'top-left': 2,
                    'top-right': 16,
                    'bottom-right': 16,
                    'bottom-left': 16,
                },
                stroke: {
                    width: 1,
                    color: greetingsPalette.ns.border,
                },
            },
        }),
    };
}
