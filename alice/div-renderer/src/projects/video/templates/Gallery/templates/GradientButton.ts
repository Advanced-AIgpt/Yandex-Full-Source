import { GradientBackground, SolidBackground, Template, TextBlock, WrapContentSize } from 'divcard2';

export function GradientButton() {
    return new TextBlock({
        text: new Template('button_text'),
        font_size: 14,
        font_weight: 'medium',
        border: {
            corner_radius: 24,
            has_shadow: 0,
        },
        margins: {
            bottom: 5,
            left: 5,
            right: 5,
            top: 5,
        },
        width: new WrapContentSize(),
        paddings: {
            bottom: 11,
            top: 11,
            left: 16,
            right: 16,
        },
        action: {
            url: new Template('button_action'),
            log_id: new Template('button_log_id'),
            payload: new Template('action_payload'),
        },
        text_alignment_horizontal: 'center',
        text_color: '#ffffff',
        background: [
            new SolidBackground({
                color: '#333436',
            }),
        ],
        extensions: [
            {
                id: 'request_focus',
            },
        ],
        focus: {
            background: [
                new GradientBackground({
                    angle: 190,
                    colors: [
                        '#F19743',
                        '#AF44C4',
                        '#5259DC',
                        '#505ADD',
                    ],
                }),
            ],
        },
        focused_text_color: '#ffffff',
    });
}
