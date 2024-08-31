import { DivFadeTransition, FixedSize, Template, TextBlock } from 'divcard2';

export function Subscription() {
    return new TextBlock({
        transition_in: new DivFadeTransition({
            duration: 300,
            start_delay: 150,
            alpha: 0.0,
            interpolator: 'ease_out',
        }),
        transition_out: new DivFadeTransition({
            duration: 150,
            alpha: 0.0,
            interpolator: 'ease_in',
        }),
        font_size: 14,
        font_weight: 'regular',
        text_color: '#b3b3b3',
        max_lines: 1,
        truncate: 'end',
        width: new FixedSize({
            value: 400,
        }),
        margins: {
            left: 20,
            right: 20,
            top: 18,
        },
        text: new Template('text'),
    });
}
