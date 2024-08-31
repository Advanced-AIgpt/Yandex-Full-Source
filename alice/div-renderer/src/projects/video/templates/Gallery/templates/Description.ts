import { DivFadeTransition, FixedSize, Template, TextBlock } from 'divcard2';

export function Description() {
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
        font_size: 16,
        font_weight: 'regular',
        line_height: 20,
        text_color: '#ffffff',
        max_lines: 3,
        truncate: 'end',
        width: new FixedSize({
            value: 400,
        }),
        height: new FixedSize({
            value: 70,
        }),
        margins: {
            left: 20,
            right: 20,
            top: 3,
        },
        text: new Template('text'),
    });
}
