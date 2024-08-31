import { getStaticS3Asset } from '../../helpers/assets';
import { NAlice } from '../../../../protos';

export const SMART_HOME_SHUTTER_CORNER_RADIUS = 28;
export const SMART_HOME_SHUTTER_IMG_SIZE = 77;
export const SMART_HOME_SHUTTER_M_OFFSET = 28;
export const SMART_HOME_SHUTTER_CARD_SIZE = SMART_HOME_SHUTTER_IMG_SIZE + 2 * SMART_HOME_SHUTTER_M_OFFSET;
export const SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION = Math.floor(SMART_HOME_SHUTTER_CARD_SIZE * 0.1);
export const SMART_HOME_SHUTTER_LINE_HEIGHT = SMART_HOME_SHUTTER_CARD_SIZE +
    2 * SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION;
export const SMART_HOME_SHUTTER_STATUS_OFFLINE = 2;
export const SMART_HOME_SHUTTER_REDIRECT_BUTTON_HEIGHT = SMART_HOME_SHUTTER_LINE_HEIGHT -
    (SMART_HOME_SHUTTER_OFFSET_FOR_ANIMATION * 2);

export enum capabilityState {
    on = 'on',
    off = 'off',
}

const iconsMap : { [deviceType in NAlice.EUserDeviceType ] ?: string } = {
    [NAlice.EUserDeviceType.LightDeviceType]: 'smarthome-icons/lamp.png',
    [NAlice.EUserDeviceType.SocketDeviceType]: 'smarthome-icons/socket.png',
    [NAlice.EUserDeviceType.SwitchDeviceType]: 'smarthome-icons/switch.png',
    [NAlice.EUserDeviceType.HubDeviceType]: 'smarthome-icons/socket.png',
    [NAlice.EUserDeviceType.PurifierDeviceType]: 'smarthome-icons/airpurifier.png',
    [NAlice.EUserDeviceType.HumidifierDeviceType]: 'smarthome-icons/humidifier.png',
    [NAlice.EUserDeviceType.VacuumCleanerDeviceType]: 'smarthome-icons/vaccleaner.png',
    [NAlice.EUserDeviceType.CookingDeviceType]: 'smarthome-icons/multicooker.png',
    [NAlice.EUserDeviceType.KettleDeviceType]: 'smarthome-icons/teapot.png',
    [NAlice.EUserDeviceType.CoffeeMakerDeviceType]: 'smarthome-icons/coffeemachine.png',
    [NAlice.EUserDeviceType.ThermostatDeviceType]: 'smarthome-icons/thermostat.png',
    [NAlice.EUserDeviceType.AcDeviceType]: 'smarthome-icons/conditioner.png',
    [NAlice.EUserDeviceType.TvDeviceDeviceType]: 'smarthome-icons/tv.png',
    [NAlice.EUserDeviceType.TvBoxDeviceType]: 'smarthome-icons/tv-dongle.png',
    [NAlice.EUserDeviceType.WashingMachineDeviceType]: 'smarthome-icons/washing-machine.png',
    [NAlice.EUserDeviceType.OpenableDeviceType]: 'smarthome-icons/openable.png',
    [NAlice.EUserDeviceType.CurtainDeviceType]: 'smarthome-icons/curtain.png',
    [NAlice.EUserDeviceType.SmartSpeakerDeviceType]: 'smarthome-icons/speaker.png',
    [NAlice.EUserDeviceType.YandexStationDeviceType]: 'smarthome-icons/random-station.png',
    [NAlice.EUserDeviceType.YandexStation2DeviceType]: 'smarthome-icons/random-station.png',
    [NAlice.EUserDeviceType.YandexStationMiniDeviceType]: 'smarthome-icons/random-station.png',
    [NAlice.EUserDeviceType.DishwasherDeviceType]: 'smarthome-icons/dishwasher.png',
    [NAlice.EUserDeviceType.MulticookerDeviceType]: 'smarthome-icons/multicooker.png',
    [NAlice.EUserDeviceType.RefrigeratorDeviceType]: 'smarthome-icons/fridge.png',
    [NAlice.EUserDeviceType.FanDeviceType]: 'smarthome-icons/fan.png',
    [NAlice.EUserDeviceType.IronDeviceType]: 'smarthome-icons/iron.png',
    [NAlice.EUserDeviceType.SensorDeviceType]: 'smarthome-icons/sensor.png',
    [NAlice.EUserDeviceType.PetFeederDeviceType]: 'smarthome-icons/feeder.png',
    [NAlice.EUserDeviceType.YandexStationCentaurDeviceType]: 'smarthome-icons/random-station.png',
    [NAlice.EUserDeviceType.LightCeilingDeviceType]: 'smarthome-icons/luster.png',
    [NAlice.EUserDeviceType.LightLampDeviceType]: 'smarthome-icons/table-lamp.png',
};

export const getDeviceIcon = (device : NAlice.TIoTUserInfo.ITDevice) : string => {
    const link = device.Type && iconsMap[device.Type];
    return link ? getStaticS3Asset(link) : `${device.IconURL}/orig`;
};
