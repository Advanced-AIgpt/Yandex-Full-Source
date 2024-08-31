import {
    ContainerBlock,
    Div,
    FixedSize,
    MatchParentSize,
    TextBlock,
    WrapContentSize,
    SolidBackground,
    DivStateBlock,
} from 'divcard2';
import { colorMoreThenBlack, offsetFromEdgeOfScreen, colorPurple } from '../../../../style/constants';
import { text32r, text40m } from '../../../../style/Text/Text';
import { ITeaserItemsListItem } from '../types';
import { compact } from 'lodash';
import { setStateActionInAllPlaces, setVariable } from '../../../../../../common/actions/div';

const imageSize = 66;
const imageCornerRadius = 24;

export function ItemsListItem(item: ITeaserItemsListItem): Div {
    const varName = (item.teaserType + item.teaserId).replace(/-/gi, '');

    let items: Div[] = [];

    if(item.teaserType == 'PhotoFrame') {
        items = [
            new TextBlock({
                width: new FixedSize({ value: imageSize }),
                height: new FixedSize({ value: imageSize }),
                margins: {
                    right: 32,
                },
                paddings: {
                    left: 10,
                    top: 10,
                    right: 10,
                    bottom: 10,
                },
                border: {
                    corner_radius: imageCornerRadius,
                },
                background: [
                    new SolidBackground({ color: colorPurple }),
                ],
                ...text32r,
                text_color: colorMoreThenBlack,
                text: 'on',
            }),
            new ContainerBlock({
                items: [
                    new TextBlock({
                        ...text40m,
                        text: item.title,
                    }),
                ],
                orientation: 'vertical',
            }),
        ];
    } else {
        items = [
            new DivStateBlock({
                width: new FixedSize({ value: imageSize }),
                height: new FixedSize({ value: imageSize }),
                margins: {
                    right: 32,
                },
                border: {
                    corner_radius: imageCornerRadius,
                },
                div_id: varName + 'teaser_control_id',
                default_state_id: item.isChosen ? (varName + 'teaser_enabled_id') : (varName + 'teaser_disabled_id'),
                states: [
                    {
                        state_id: varName + 'teaser_enabled_id',
                        div: new TextBlock({
                            width: new FixedSize({ value: imageSize }),
                            height: new FixedSize({ value: imageSize }),
                            paddings: {
                                left: 10,
                                top: 10,
                                right: 10,
                                bottom: 10,
                            },
                            background: [
                                new SolidBackground({ color: colorPurple }),
                            ],
                            ...text32r,
                            text_color: colorMoreThenBlack,
                            text: 'on',
                            actions: compact([
                                {
                                    url: setVariable(varName, 0),
                                    log_id: varName + 'teaser_set_action_on',
                                },
                            ]),
                        }),
                    },
                    {
                        state_id: varName + 'teaser_disabled_id',
                        div: new TextBlock({
                            width: new FixedSize({ value: imageSize }),
                            height: new FixedSize({ value: imageSize }),
                            paddings: {
                                left: 10,
                                top: 10,
                                right: 10,
                                bottom: 10,
                            },
                            background: [
                                new SolidBackground({ color: '#ffffff' }),
                            ],
                            ...text32r,
                            text_color: colorMoreThenBlack,
                            text: 'off',
                            actions: compact([
                                {
                                    url: setVariable(varName, 1),
                                    log_id: varName + 'teaser_set_action_off',
                                },
                            ]),
                        }),
                    },
                ],
            }),
            new ContainerBlock({
                items: [
                    new TextBlock({
                        ...text40m,
                        text: item.title,
                    }),
                ],
                orientation: 'vertical',
            }),
        ];
    }

    const containerProps: ConstructorParameters<typeof ContainerBlock>[0] = {
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        paddings: {
            top: 32,
            bottom: 34,
        },
        items,
    };

    return new ContainerBlock(containerProps);
}

export function teaserTriggers(name: string) {
    return [
        {
            condition: `@{${name} == 1.0}`,
            actions: setStateActionInAllPlaces(
                {
                    places: [{ name: 'top_level', place: ['0'] }],
                    state: [
                        name + 'teaser_control_id',
                        name + 'teaser_enabled_id',
                    ],
                    logPrefix: 'teasers_set_on_',
                },
            ),
        },
        {
            condition: `@{${name} == 0.0}`,
            actions: setStateActionInAllPlaces(
                {
                    places: [{ name: 'top_level', place: ['0'] }],
                    state: [
                        name + 'teaser_control_id',
                        name + 'teaser_disabled_id',
                    ],
                    logPrefix: 'teasers__set_off_',
                },
            ),
        },
    ];
}

