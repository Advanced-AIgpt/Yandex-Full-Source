import {
    ContainerBlock, FixedSize,
    GalleryBlock,
    MatchParentSize, SolidBackground,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { title32m } from '../../style/Text/Text';
import { colorWhiteOpacity10, colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../style/constants';
import EmptyDiv from '../../components/EmptyDiv';
import { SmartHomeRedirectButton } from './components/SmartHomeRedirectButton';
import { StateDeviceCard } from './components/StateDeviceCard';
import {
    SMART_HOME_SHUTTER_LINE_HEIGHT,
    SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION,
} from './constants';

export const renderSmartHomeUpperShutter = ({
    Id,
    IoTUserData,
}: NAlice.NData.ITCentaurMainScreenSmartHomeTabData) =>
    renderUpperShutter({ Id, SmartHomeData: { IoTUserData } });

export const CapabilityType = NAlice.TIoTUserInfo.TCapability.ECapabilityType;
export const renderUpperShutter = ({
    Id,
    SmartHomeData,
}: NAlice.NData.ITCentaurUpperShutterData) => {
    const devices = (
        SmartHomeData?.IoTUserData?.Devices?.length &&
        filterDevices(SmartHomeData.IoTUserData.Devices)
    ) || [];

    return new TemplateCard(new Templates({}), {
        log_id: Id ?? '',
        states: [
            {
                state_id: 0,
                div: typeof devices !== 'undefined' && devices.length > 0 ? new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new WrapContentSize(),
                    items: [
                        new TextBlock({
                            ...title32m,
                            text_color: colorWhiteOpacity50,
                            text: 'Умный дом',
                            margins: {
                                left: offsetFromEdgeOfScreen,
                            },
                        }),
                        new GalleryBlock({
                            margins: {
                                top: Math.max(48 - SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION, 0),
                            },
                            width: new MatchParentSize(),
                            height: new FixedSize({ value: SMART_HOME_SHUTTER_LINE_HEIGHT }),
                            cross_content_alignment: 'center',
                            items: compact([
                                ...devices.map(
                                    (device, index) => StateDeviceCard(
                                        device, SmartHomeData?.IoTUserData, index === 0)) ?? [],
                                devices.length > 0 && new EmptyDiv({
                                    width: new FixedSize({ value: 4 }),
                                    height: new FixedSize({ value: 24 }),
                                    background: [
                                        new SolidBackground({ color: colorWhiteOpacity10 }),
                                    ],
                                }),
                                SmartHomeRedirectButton(devices.length === 0),
                            ]),
                        }),
                    ],
                }) : new EmptyDiv(),
            },
        ],
    });
};

const filterDevices = (
    devices: NAlice.TIoTUserInfo.ITDevice[],
) =>
    devices?.filter(
        device =>
            device.Capabilities?.some(
                capability => capability.Type === CapabilityType.OnOffCapabilityType,
            ),
    );
