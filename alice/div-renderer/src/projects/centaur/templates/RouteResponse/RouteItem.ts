import { text28m, title64m } from '../../style/Text/Text';
import { EnumTravelTypes, IRoute } from './types';
import {
    ContainerBlock,
    Div, DivFadeTransition,
    FixedSize,
    IDivAction, ImageBlock,
    MatchParentSize,
    TextBlock, WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { colorGrey, colorWhite, offsetFromEdgeOfScreen } from '../../style/constants';
import { getStaticS3Asset } from '../../helpers/assets';

const verticalPadding = 44;

function getImageByType(type: EnumTravelTypes): string {
    switch (type) {
        case EnumTravelTypes.car:
            return getStaticS3Asset('icons/route_response/car.png');
        case EnumTravelTypes.undefined:
            return getStaticS3Asset('icons/route_response/undef.png');
        case EnumTravelTypes.onFoot:
            return getStaticS3Asset('icons/route_response/pedestrian.png');
        case EnumTravelTypes.publicTransport:
            return getStaticS3Asset('icons/route_response/bus.png');
    }
}

export default function RouteItem(
    data: IRoute,
    actions: IDivAction[] | undefined,
    isActive = false,
    options: { height?: number; length: number; } = {
        length: 3,
    },
): Div {
    const iconSize = 64;
    const iconInnerSize = 38;

    return new ContainerBlock({
        actions,
        id: `${data.type}${isActive ? 'active' : 'not_active'}`,
        height: options.height ? new FixedSize({ value: options.height }) : new WrapContentSize(),
        content_alignment_vertical: 'center',
        orientation: 'horizontal',
        alpha: isActive ? 1 : 0.5,
        transition_in: new DivFadeTransition({
            duration: 500,
            alpha: isActive ? 0.5 : 1,
            interpolator: 'ease_in_out',
        }),
        transition_out: new DivFadeTransition({
            duration: 500,
            alpha: isActive ? 0.5 : 1,
            interpolator: 'ease_in_out',
        }),
        items: compact([
            new ContainerBlock({
                id: `${data.type}_element_${isActive ? 'active' : ''}`,
                paddings: {
                    top: verticalPadding,
                    left: offsetFromEdgeOfScreen,
                    bottom: verticalPadding,
                    right: 117,
                },
                width: new MatchParentSize({ weight: 1 }),
                items: compact([
                    new ContainerBlock({
                        orientation: 'horizontal',
                        content_alignment_vertical: 'center',
                        items: [
                            new ContainerBlock({
                                orientation: 'overlap',
                                content_alignment_vertical: 'center',
                                content_alignment_horizontal: 'center',
                                width: new FixedSize({ value: iconSize }),
                                height: new FixedSize({ value: iconSize }),
                                transition_change: {
                                    type: 'change_bounds',
                                    duration: 500,
                                },
                                margins: {
                                    right: 24,
                                },
                                items: [
                                    new ImageBlock({
                                        image_url: getStaticS3Asset('icons/route_response/icon_bg.png'),
                                        width: new FixedSize({ value: iconSize }),
                                        height: new FixedSize({ value: iconSize }),
                                        tint_color: colorWhite,
                                    }),
                                    new ImageBlock({
                                        image_url: getImageByType(data.type),
                                        width: new FixedSize({ value: iconInnerSize }),
                                        height: new FixedSize({ value: iconInnerSize }),
                                        tint_color: colorGrey,
                                    }),
                                ],
                            }),
                            new TextBlock({
                                ...title64m,
                                text: data.value,
                                id: `${data.type}remote_item_div_title`,
                            }),
                        ],
                    }),
                    data.comment && new TextBlock({
                        ...text28m,
                        text: data.comment,
                        id: `${data.type}remote_item_div_content`,
                    }),
                ]),
            }),
        ]),
    });
}
