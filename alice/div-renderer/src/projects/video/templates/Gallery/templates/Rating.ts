import { DivFadeTransition, Template, TextBlock } from 'divcard2';

export function Rating() {
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
        font_weight: 'medium',
        max_lines: 1,
        text: new Template('text'),
    });
}
