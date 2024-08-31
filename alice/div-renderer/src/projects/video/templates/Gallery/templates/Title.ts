import { DivFadeTransition, MatchParentSize, Template, TextBlock } from 'divcard2';

export function Title() {
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
        font_size: 36,
        font_weight: 'bold',
        line_height: 40,
        text_color: '#ffffff',
        alignment_horizontal: 'left',
        text_alignment_horizontal: 'left',
        text_alignment_vertical: 'bottom',
        truncate: 'end',
        max_lines: 2,
        height: new MatchParentSize(),
        margins: {
            left: 20,
            top: 23,
        },
        text: new Template('text'),
    });
}
