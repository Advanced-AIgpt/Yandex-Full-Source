import { createSemanticFrameAction, createSemanticFrameActionTypeSafe } from '.';
import { Directive } from '..';
import { TTypedSemanticFrame } from '../../../protos/alice/megamind/protos/common/frame';

export enum OnOffIotDeviceActionType {
    TurnOn,
    TurnOff
}

export function onOffIotDeviceServerAction(deviceId: string, onOffActionType: OnOffIotDeviceActionType): Directive {
    return createSemanticFrameAction(
        {
            iot_device_action_semantic_frame: {
                request: {
                    request_value: {
                        intent_parameters: {
                            capability_instance: 'on',
                            capability_value: {
                                bool_value: onOffActionType === OnOffIotDeviceActionType.TurnOn,
                            },
                            capability_type: 'devices.capabilities.on_off',
                        },
                        device_ids: [
                            deviceId,
                        ],
                    },
                },
            },
        },
        'IOT',
        `Turn ${onOffActionType === OnOffIotDeviceActionType.TurnOn ? 'On' : 'Off'} iot device`,
    );
}

// Сейчас не используется - демонстрация type-safe server_action на ts-proto
export function onOffIotDeviceServerActionTypeSafe(deviceId: string, onOffActionType: OnOffIotDeviceActionType): Directive {
    return createSemanticFrameActionTypeSafe(
        {
            IoTDeviceActionSemanticFrame: {
                Request: {
                    RequestValue: {
                        IntentParameters: {
                            CapabilityInstance: 'on',
                            CapabilityValue: {
                                RelativityType: '',
                                Unit: '',
                                NumValue: undefined,
                                ModeValue: undefined,
                                BoolValue: onOffActionType === OnOffIotDeviceActionType.TurnOn,
                            },
                            CapabilityType: 'devices.capabilities.on_off',
                        },
                        DeviceIDs: [deviceId],
                        RoomIDs: [],
                        HouseholdIDs: [],
                        GroupIDs: [],
                        DeviceTypes: [],
                        AtTimestamp: 0,
                        FromTimestamp: 0,
                        ToTimestamp: 0,
                        ForTimestamp: 0,
                    },
                },
            },
        } as TTypedSemanticFrame,
        'IOT',
        `Turn ${onOffActionType === OnOffIotDeviceActionType.TurnOn ? 'On' : 'Off'} iot device`,
    );
}
