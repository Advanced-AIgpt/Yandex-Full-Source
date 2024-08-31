import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';
import {
    ContainerBlock,
    Div,
    DivStateBlock, DivVariable,
    FixedSize,
    ImageBlock,
    MatchParentSize, SeparatorBlock,
    SolidBackground,
    TextBlock,
} from 'divcard2';
import { text28m } from '../../style/Text/Text';
import { DualScreen, DualScreenStates } from '../../components/DualScreen/DualScreen';
import { getButtonListVariableId } from '../../components/ButtonsLikeRadio/ButtonsLikeRadio';
import { colorWhite, colorWhiteOpacity0, offsetFromEdgeOfScreen } from '../../style/constants';
import VariableTab from '../../components/VariableTab/VariableTab';
import { IRouteResponseData } from './types';
import RouteItem from './RouteItem';
import { dataAdapter } from './RouteResponseDataAdapter';
import { NAlice } from '../../../../protos';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import RouteResponseEmpty from './RouteResponseEmpty';
import { setStateActionInAllPlaces, setVariable } from '../../../../common/actions/div';
import { compact } from 'lodash';
type ITShowRouteData = NAlice.NData.ITShowRouteData;

const placesDualScreen = [
    {
        name: 'in_dual_screen_horizontal',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateHorizontal,
        ],
    },
    {
        name: 'in_dual_screen_vertical',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateVertical,
        ],
    },
];

const titleHeight = 180;

function RouteItemsWithActivity(data: IRouteResponseData, requestState: IRequestState, active: number) {
    const activeItemHeight = Math.ceil((requestState.sizes.height - titleHeight) / data.routes.length);
    const notActiveItemHeight = activeItemHeight;
    return new ContainerBlock({
        orientation: 'overlap',
        items: compact([
            data.routes.length > 1 && new ContainerBlock({
                id: 'active_item',
                background: [new SolidBackground({ color: '#0DE6EEF9' })],
                height: new FixedSize({ value: activeItemHeight }),
                content_alignment_vertical: 'center',
                orientation: 'horizontal',
                margins: {
                    top: notActiveItemHeight * active,
                },
                transition_change: {
                    type: 'change_bounds',
                    duration: 500,
                },
                items: compact([
                    new SeparatorBlock({
                        height: new MatchParentSize({ weight: 1 }),
                        width: new FixedSize({ value: 6 }),
                        background: [new SolidBackground({ color: colorWhite })],
                        border: {
                            corners_radius: {
                                'top-right': 6,
                                'bottom-right': 6,
                            },
                        },
                        delimiter_style: {
                            orientation: 'vertical',
                            color: colorWhiteOpacity0,
                        },
                    }),
                ]),
            }),
            new ContainerBlock({
                items: data.routes.map((el, index) => RouteItem(
                    el,
                    index === active ? [] : [
                        {
                            log_id: `Change active route item to ${index}`,
                            url: setVariable(getButtonListVariableId('change_routes'), index),
                        },
                    ],
                    index === active,
                    {
                        length: data.routes.length,
                        height: index === active ? activeItemHeight : notActiveItemHeight,
                    } )),
            }),
        ]),
    });
}

const routeVariable: DivVariable = {
    name: getButtonListVariableId('change_routes'),
    value: 0,
    type: 'number',
};

function RouteResponseDiv(data: IRouteResponseData, requestState: IRequestState): Div {
    requestState.variables.add(routeVariable);

    return CloseButtonWrapper({
        closeButtonProps: {
            borderRadius: 100,
            backgroundColor: data.colorSet.suggestsBackground,
        },
        div: DualScreen({
            firstDiv: [
                new ContainerBlock({
                    items: [
                        new ContainerBlock({
                            height: new FixedSize({ value: titleHeight }),
                            paddings: {
                                top: offsetFromEdgeOfScreen,
                                left: offsetFromEdgeOfScreen,
                                bottom: offsetFromEdgeOfScreen,
                                right: offsetFromEdgeOfScreen,
                            },
                            items: [
                                new TextBlock({
                                    ...text28m,
                                    text: `Откуда: ${data.from}`,
                                    alpha: 0.5,
                                    margins: {
                                        bottom: 12,
                                    },
                                }),
                                new TextBlock({
                                    ...text28m,
                                    text: `Куда: ${data.to}`,
                                }),
                            ],
                        }),
                        new DivStateBlock({
                            div_id: 'route_buttons',
                            transition_animation_selector: 'any_change',
                            states: data.routes.map((_, index) => {
                                requestState.variableTriggers.add({
                                    condition: `@{${getButtonListVariableId('change_routes')} == ${index}.0}`,
                                    actions: setStateActionInAllPlaces({
                                        places: placesDualScreen,
                                        state: ['route_buttons', `active_${index}`],
                                        logPrefix: `set active route item to ${index}`,
                                    }),
                                });

                                return {
                                    state_id: `active_${index}`,
                                    div: RouteItemsWithActivity(data, requestState, index),
                                };
                            }),
                        }),
                    ],
                }),
            ],
            secondDiv: [
                VariableTab({
                    options: {
                        height: new MatchParentSize({ weight: 1 }),
                        width: new MatchParentSize({ weight: 1 }),
                    },
                    items: data.routes.map(el => new ImageBlock({
                        image_url: el.map,
                        scale: 'fill',
                        width: new MatchParentSize({ weight: 1 }),
                        height: new MatchParentSize({ weight: 1 }),
                    })),
                    places: placesDualScreen,
                    variableName: getButtonListVariableId('change_routes'),
                    stateName: 'route_map',
                    requestState,
                }),
            ],
            requestState,
            mainColor: data.colorSet.mainColor,
            mainColor1: data.colorSet.mainColor1,
        }),
    });
}

export default function RouteResponse(data: ITShowRouteData, _:MMRequest, requestState: IRequestState) {
    const normalizedData = dataAdapter(data, requestState);

    return TopLevelCard({
        log_id: 'route_response',
        transition_animation_selector: 'any_change',
        states: [
            {
                state_id: 0,
                div: normalizedData.routes.length > 0 ? RouteResponseDiv(normalizedData, requestState) : RouteResponseEmpty(requestState),
            },
        ],
    }, requestState);
}
