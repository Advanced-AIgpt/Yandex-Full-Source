import {
    ContainerBlock,
    DivFadeTransition,
    FixedSize, GradientBackground,
    ImageBlock,
    MatchParentSize,
    Template,
    TextBlock,
    WrapContentSize,
} from 'divcard2';

export function PromoItem() {
    return new ContainerBlock({
        orientation: 'overlap',
        height: new MatchParentSize(),
        items: [
            new ImageBlock({
                id: new Template('thumbnail_id'),
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
            }),
            new ContainerBlock({
                content_alignment_horizontal: 'left',
                content_alignment_vertical: 'bottom',
                height: new MatchParentSize(),
                items: [
                    new ContainerBlock({
                        orientation: 'overlap',
                        content_alignment_horizontal: 'left',
                        content_alignment_vertical: 'bottom',
                        height: new MatchParentSize(),
                        width: new FixedSize({
                            value: 400,
                        }),
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
                                id: new Template('title_id'),
                                text: new Template('title_text'),
                            }),
                            new ImageBlock({
                                id: 'logo_id',
                                image_url: new Template('logo_url'),
                                alpha: new Template('has_logo'),
                                alignment_horizontal: 'left',
                                content_alignment_horizontal: 'left',
                                alignment_vertical: 'bottom',
                                content_alignment_vertical: 'bottom',
                                height: new MatchParentSize(),
                                scale: 'fit',
                                margins: {
                                    left: 20,
                                    right: 20,
                                    top: 23,
                                },
                                extensions: [{
                                    id: 'transition',
                                    params: {
                                        name: 'logo',
                                        target: 'logo',
                                    },
                                }],
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
                            }),
                        ],
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        width: new WrapContentSize(),
                        margins: {
                            left: 20,
                            right: 20,
                            top: 12,
                        },
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
                                font_size: 14,
                                font_weight: 'medium',
                                max_lines: 1,
                                id: new Template('rating_id'),
                                text: new Template('rating_text'),
                                text_color: new Template('rating_color'),
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
                                font_size: 14,
                                font_weight: 'medium',
                                text_color: '#b3b3b3',
                                max_lines: 1,
                                truncate: 'end',
                                width: new FixedSize({
                                    value: 400,
                                }),
                                text: new Template('meta_text'),
                                id: new Template('meta_id'),
                            }),
                        ],
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
                            top: 3,
                        },
                        id: new Template('description'),
                        text: new Template('description_text'),
                    }),
                    new TextBlock({
                        id: new Template('subscription_id'),
                        text: new Template('subscription_text'),
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
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        margins: {
                            top: 7,
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
                            '#02151517',
                            '#02151517',
                            '#03151517',
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
                            '#02151517',
                            '#02151517',
                        ],
                    }),
                ],
            }),
            new ImageBlock({
                alignment_horizontal: 'right',
                content_alignment_horizontal: 'right',
                alignment_vertical: 'top',
                content_alignment_vertical: 'top',
                width: new WrapContentSize(),
                height: new FixedSize({
                    value: 60,
                }),
                scale: 'fit',
                margins: {
                    right: 30,
                    top: 52,
                },
                id: new Template('legal_logo_id'),
                image_url: new Template('legal_logo_url'),
                alpha: new Template('has_legal_logo'),
            }),
        ],
    });
}
