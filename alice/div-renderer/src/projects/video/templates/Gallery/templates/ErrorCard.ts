import {
    ContainerBlock,
    DivFadeTransition,
    FixedSize,
    GradientBackground,
    ImageBlock,
    MatchParentSize,
    Template,
    TextBlock,
} from 'divcard2';

export function ErrorCard() {
    return new ContainerBlock({
        orientation: 'overlap',
        height: new MatchParentSize(),
        items: [
            new ImageBlock({
                width: new FixedSize({
                    value: 700,
                }),
                height: new FixedSize({
                    value: 400,
                }),
                extensions: [
                    {
                        id: 'transition',
                        params: {
                            name: 'thumbnail',
                            target: 'thumbnail',
                        },
                    },
                ],
                alignment_horizontal: 'right',
                alignment_vertical: 'top',
                transition_in: new DivFadeTransition({
                    duration: 400,
                    alpha: 0.0,
                    interpolator: 'ease_out',
                }),
                image_url: new Template('thumbnail_url'),
                id: new Template('thumbnail_id'),
            }),
            new ContainerBlock({
                content_alignment_horizontal: 'left',
                content_alignment_vertical: 'bottom',
                height: new MatchParentSize(),
                items: [
                    new TextBlock({
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
                        text: new Template('title_text'),
                    }),
                    new TextBlock({
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
                            top: 12,
                        },
                        id: new Template('description_id'),
                        text: new Template('description_text'),
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        margins: {
                            top: 66,
                            bottom: 232,
                            left: 15,
                            right: 15,
                        },
                        items: new Template('btns'),
                    }),
                ],
                background: [
                    new GradientBackground({
                        angle: 0,
                        colors: [
                            '#ff151517',
                            '#ff151517',
                            '#ff151517',
                            '#c0151517',
                            '#80151517',
                            '#30151517',
                            '#00151517',
                            '#00151517',
                            '#00151517',
                        ],
                    }),
                    new GradientBackground({
                        angle: 90,
                        colors: [
                            '#ff151517',
                            '#ff151517',
                            '#ff151517',
                            '#b0151517',
                            '#70151517',
                            '#10151517',
                            '#00151517',
                            '#00151517',
                        ],
                    }),
                ],
            }),
        ],
    });
}
