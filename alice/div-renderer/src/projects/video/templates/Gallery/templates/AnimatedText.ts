import { DivFadeTransition, Template, TextBlock } from 'divcard2';

export function AnimatedText() {
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
        text: new Template('text'),
    });
}
