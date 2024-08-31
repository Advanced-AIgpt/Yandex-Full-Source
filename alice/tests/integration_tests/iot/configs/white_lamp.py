from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.device.info_pb2 import EUserDeviceType
from alice.protos.data.location.room_pb2 import TUserRoom

TCapability = TIoTUserInfo.TCapability
TProperty = TIoTUserInfo.TProperty
ECapabilityType = TCapability.ECapabilityType

white_lamp = TIoTUserInfo(
    RawUserInfo='{}',
    Scenarios=[],
    Colors=[
        TIoTUserInfo.TColor(
            Id='cold_white',
            Name='Холодный белый',
        ),
    ],
    Rooms=[
        TUserRoom(
            Id='room-test-id-1',
            Name='Кухня',
            HouseholdId='evo-test-household-id-1',
        ),
    ],
    Households=[
        TIoTUserInfo.THousehold(
            Id='evo-test-household-id-1',
            Name='Дом',
        ),
    ],
    CurrentHouseholdId='evo-test-household-id-1',
    Groups=[],
    Devices=[
        TIoTUserInfo.TDevice(
            HouseholdId='evo-test-household-id-1',
            Id='evo-test-lamp-id-1',
            ExternalId='evo-test-lamp-external-id-1',
            Name='Лампочка',
            Aliases=[],
            SkillId='QUALITY',
            Status=TIoTUserInfo.TDevice.EDeviceState.OnlineDeviceState,
            Type=EUserDeviceType.LightDeviceType,
            OriginalType=EUserDeviceType.LightDeviceType,
            AnalyticsType='devices.types.light',
            RoomId='room-test-id-1',
            Capabilities=[
                TCapability(
                    Type=ECapabilityType.OnOffCapabilityType,
                    OnOffCapabilityParameters=TCapability.TOnOffCapabilityParameters(),
                    OnOffCapabilityState=TCapability.TOnOffCapabilityState(
                        Instance='on',
                        Value=True,
                    ),
                    AnalyticsType='devices.capabilities.on_off',
                    Retrievable=True,
                    Reportable=True,
                ),
                TCapability(
                    Type=ECapabilityType.ColorSettingCapabilityType,
                    ColorSettingCapabilityParameters=TCapability.TColorSettingCapabilityParameters(
                        TemperatureK=TCapability.TColorSettingCapabilityParameters.TTemperatureKCapabilityParameters(
                            Min=2000,
                            Max=9000,
                        ),
                        ColorSceneParameters=TCapability.TColorSettingCapabilityParameters.TColorSceneParameters(),
                    ),
                    ColorSettingCapabilityState=TCapability.TColorSettingCapabilityState(
                        Instance='temperature_k',
                        TemperatureK=4000,
                    ),
                    AnalyticsType='devices.capabilities.color_setting',
                    Retrievable=True,
                    Reportable=False,
                ),
            ]
        )
    ]
)
