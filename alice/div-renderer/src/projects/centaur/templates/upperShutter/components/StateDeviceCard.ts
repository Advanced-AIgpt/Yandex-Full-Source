import { DivStateBlock, FixedSize, WrapContentSize } from 'divcard2';
import { NAlice } from '../../../../../protos';
import {
    capabilityState,
    SMART_HOME_SHUTTER_LINE_HEIGHT,
} from '../constants';
import { tappableAnimation } from '../../../style/constants';
import { DeviceCard } from './DeviceCard';
import { CapabilityType } from '../upperShutter';

export const StateDeviceCard = (
    device: NAlice.TIoTUserInfo.ITDevice,
    IoTUserData: NAlice.ITIoTUserInfo | null | undefined,
    isFirst: boolean = false,
) => {
    return new DivStateBlock({
        div_id: `device${device.Id}`,
        width: new WrapContentSize(),
        height: new FixedSize({ value: SMART_HOME_SHUTTER_LINE_HEIGHT }),
        default_state_id: isDeviceOn(device) ? capabilityState.on : capabilityState.off,
        states: [
            {
                state_id: capabilityState.on,
                div: DeviceCard(device, IoTUserData, isFirst, true),
                ...tappableAnimation,
            },
            {
                state_id: capabilityState.off,
                div: DeviceCard(device, IoTUserData, isFirst, false),
                ...tappableAnimation,
            },
        ],
    });
};

function isDeviceOn(device: NAlice.TIoTUserInfo.ITDevice) {
    return device.Capabilities?.find(
        capability => capability.Type === CapabilityType.OnOffCapabilityType,
    )?.OnOffCapabilityState?.Value;
}
