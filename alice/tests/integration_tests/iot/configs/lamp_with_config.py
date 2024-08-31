from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.device.info_pb2 import EUserDeviceType

TCapability = TIoTUserInfo.TCapability
TProperty = TIoTUserInfo.TProperty
ECapabilityType = TCapability.ECapabilityType

lamp = TIoTUserInfo(
    RawUserInfo='{}',
    Scenarios=[],
    Colors=[],
    Rooms=[],
    Groups=[],
    Devices=[
        TIoTUserInfo.TDevice(
            Id='evo-test-lamp-id-1',
            ExternalId='evo-test-lamp-external-id-1',
            Name='Лампочка',
            Aliases=[],
            SkillId='QUALITY',
            Type=EUserDeviceType.LightDeviceType,
            OriginalType=EUserDeviceType.LightDeviceType,
            AnalyticsType='devices.types.light',
            HouseholdId='evo-test-household-id-1',
            Capabilities=[
                TCapability(
                    Type=ECapabilityType.OnOffCapabilityType,
                    OnOffCapabilityParameters=TCapability.TOnOffCapabilityParameters(),
                    OnOffCapabilityState=TCapability.TOnOffCapabilityState(),
                    AnalyticsType='devices.capabilities.on_off',
                    Retrievable=True,
                    Reportable=True,
                )
            ]
        )
    ]
)
