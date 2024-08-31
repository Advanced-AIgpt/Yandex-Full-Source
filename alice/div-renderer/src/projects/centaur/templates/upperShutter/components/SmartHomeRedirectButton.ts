import { ContainerBlock, FixedSize, ImageBlock, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import { SMART_HOME_SHUTTER_CORNER_RADIUS, SMART_HOME_SHUTTER_M_OFFSET, SMART_HOME_SHUTTER_REDIRECT_BUTTON_HEIGHT } from '../constants';
import { colorWhiteOpacity10, offsetForScaleAnimation, offsetFromEdgeOfScreen } from '../../../style/constants';
import { title32m } from '../../../style/Text/Text';
import { getS3Asset } from '../../../helpers/assets';
import { directivesAction } from '../../../../../common/actions';
import { createClientAction } from '../../../../../common/actions/client';

export const SmartHomeRedirectButton = (
    isFirst: boolean = false,
) => {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new FixedSize({ value: SMART_HOME_SHUTTER_REDIRECT_BUTTON_HEIGHT }),
        orientation: 'horizontal',
        alignment_vertical: 'center',
        background: [new SolidBackground({
            color: colorWhiteOpacity10,
        })],
        margins: {
            right: offsetFromEdgeOfScreen,
            left: isFirst ? offsetFromEdgeOfScreen : offsetForScaleAnimation,
        },
        paddings: {
            bottom: SMART_HOME_SHUTTER_M_OFFSET,
            top: SMART_HOME_SHUTTER_M_OFFSET,
            left: 43,
            right: 48,
        },
        border: {
            corner_radius: SMART_HOME_SHUTTER_CORNER_RADIUS,
        },
        action: {
            log_id: 'upper_shutter.redirect.smart_home',
            url: directivesAction([createClientAction('show_main_screen', { tab_id: 'tab3' })]),
        },
        items: compact([
            new ImageBlock({
                image_url: getS3Asset('iot/icons/station.png'),
                height: new FixedSize({ value: 48 }),
                width: new FixedSize({ value: 32 }),
                alignment_vertical: 'center',
            }),
            new TextBlock({
                ...title32m,
                alignment_vertical: 'center',
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                text: 'Умный Дом',
                margins: {
                    left: 23,
                    right: 30,
                },
            }),
            new ImageBlock({
                image_url: getS3Asset('iot/icons/right.png'),
                height: new FixedSize({ value: 37 }),
                width: new FixedSize({ value: 20 }),
                alignment_vertical: 'center',
            }),
        ]),
    });
};
