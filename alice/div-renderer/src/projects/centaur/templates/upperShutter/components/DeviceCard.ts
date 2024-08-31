import {
    ContainerBlock,
    FixedSize,
    ImageBackground,
    MatchParentSize,
    SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import {
    colorPurple,
    colorWhiteOpacity10,
    colorWhiteOpacity50,
    disabledTextColor,
    mainTextColor,
    offsetForScaleAnimation,
    offsetFromEdgeOfScreen,
} from '../../../style/constants';
import { capabilityState, getDeviceIcon, SMART_HOME_SHUTTER_CORNER_RADIUS, SMART_HOME_SHUTTER_IMG_SIZE, SMART_HOME_SHUTTER_M_OFFSET, SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION, SMART_HOME_SHUTTER_STATUS_OFFLINE } from '../constants';
import EmptyDiv from '../../../components/EmptyDiv';
import { text28m, title32m } from '../../../style/Text/Text';
import { setStateAction } from '../../../../../common/actions/div';
import { Directive, directivesAction } from '../../../../../common/actions';
import { OnOffIotDeviceActionType, onOffIotDeviceServerAction } from '../../../../../common/actions/server/iotActions';

export const DeviceCard = (
    device: NAlice.TIoTUserInfo.ITDevice,
    IoTUserData: NAlice.ITIoTUserInfo | null | undefined,
    isFirst: boolean = false,
    isDeviceOn: boolean,
) => {
    return new ContainerBlock({
        height: new MatchParentSize(),
        width: new MatchParentSize(),
        orientation: 'horizontal',
        background: [
            new SolidBackground({
                color: (!isDeviceDisabled() && isDeviceOn) ? colorPurple : colorWhiteOpacity10,
            }),
        ],
        margins: {
            left: isFirst ? offsetFromEdgeOfScreen : offsetForScaleAnimation,
            right: offsetForScaleAnimation,
            top: SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION,
            bottom: SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION,
        },
        paddings: {
            bottom: SMART_HOME_SHUTTER_M_OFFSET,
            top: SMART_HOME_SHUTTER_M_OFFSET,
            left: SMART_HOME_SHUTTER_M_OFFSET,
            right: SMART_HOME_SHUTTER_M_OFFSET,
        },
        border: {
            corner_radius: SMART_HOME_SHUTTER_CORNER_RADIUS,
        },
        actions: !isDeviceDisabled() ? [
            {
                log_id: `upper_shutter.device.${device.Id}.change.button.state`,
                url: setStateAction(`0/device${device.Id}/${isDeviceOn ? capabilityState.off : capabilityState.on}`),
            },
            {
                log_id: `upper_shutter.device.${device.Id}.click`,
                url: directivesAction([createOnOffDeviceServerAction(isDeviceOn)]),
            },
        ] : undefined,
        items: compact([
            new EmptyDiv({
                width: new FixedSize({ value: SMART_HOME_SHUTTER_IMG_SIZE }),
                height: new FixedSize({ value: SMART_HOME_SHUTTER_IMG_SIZE }),
                background: compact([
                    device.IconURL && new ImageBackground({
                        image_url: getDeviceIcon(device),
                        preload_required: 1,
                        alpha: isDeviceDisabled() ? 0.3 : 1,
                    }),
                ]),
                border: {
                    corner_radius: SMART_HOME_SHUTTER_CORNER_RADIUS,
                },
            }),
            new ContainerBlock({
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                margins: {
                    left: 20,
                },
                alignment_vertical: 'center',
                items: compact([
                    new TextBlock({
                        ...title32m,
                        text_color: isDeviceDisabled() ? disabledTextColor : mainTextColor,
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                        text: getFormatDeviceName(),
                        max_lines: 2,
                    }),
                    new ContainerBlock({
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                        orientation: 'horizontal',
                        items: compact([
                            new TextBlock({
                                ...text28m,
                                text_color: isDeviceDisabled() ? disabledTextColor : colorWhiteOpacity50,
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                text: getDeviceRoom(),
                                max_lines: 2,
                            }),
                            new TextBlock({
                                ...text28m,
                                text_color: getStatusColor(),
                                width: new WrapContentSize(),
                                height: new WrapContentSize(),
                                text: getStatusText(),
                                max_lines: 2,
                            }),
                        ]),
                    }),
                ]),
            }),
        ]),
    });

    function createOnOffDeviceServerAction(isDeviceOn: boolean): Directive {
        return onOffIotDeviceServerAction(device.Id ?? '', isDeviceOn ? OnOffIotDeviceActionType.TurnOff : OnOffIotDeviceActionType.TurnOn);
    }

    function getFormatDeviceName() {
        if (!device.Name?.length) {
            return '';
        }
        if (device.Name.length > 25) {
            return device.Name.slice(0, 22) + '...';
        }
        return device.Name;
    }

    function getDeviceRoom() {
        const EMPTY_ROOM = isDeviceDisabled() ? '' : 'Добавить в комнату';
        if (!device.RoomId) {
            return EMPTY_ROOM;
        }

        return IoTUserData?.Rooms?.find(room => room.Id === device.RoomId)?.Name || EMPTY_ROOM;
    }

    function getStatusText() {
        // todo: предусмотреть кейсы для статуса ' · Ошибка'
        return isDeviceDisabled() ? ' · Не в сети' : '';
    }

    function getStatusColor() {
        // todo: предусмотреть кейс для статуса ошибка с цветом errorStatusColor
        return colorWhiteOpacity50;
    }

    function isDeviceDisabled() {
        return device.Status === SMART_HOME_SHUTTER_STATUS_OFFLINE;
    }
};
