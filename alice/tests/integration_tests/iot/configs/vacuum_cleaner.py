from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.device.info_pb2 import EUserDeviceType

TCapability = TIoTUserInfo.TCapability
TProperty = TIoTUserInfo.TProperty
ECapabilityType = TCapability.ECapabilityType

vacuum_cleaner = TIoTUserInfo(
    RawUserInfo='{}',
    Scenarios=[],
    Colors=[],
    Rooms=[],
    Groups=[],
    Devices=[
        TIoTUserInfo.TDevice(
            Id='evo-test-vacuum-id-1',
            ExternalId='evo-test-vacuum-external-id-1',
            Name='Аркадий',
            Aliases=[],
            SkillId='QUALITY',
            Type=EUserDeviceType.VacuumCleanerDeviceType,
            OriginalType=EUserDeviceType.VacuumCleanerDeviceType,
            AnalyticsType='devices.types.vacuum_cleaner',
            HouseholdId='evo-test-household-id-1',
            Capabilities=[
                TCapability(
                    Type=ECapabilityType.OnOffCapabilityType,
                    OnOffCapabilityParameters=TCapability.TOnOffCapabilityParameters(
                        Split=True
                    ),
                    OnOffCapabilityState=TCapability.TOnOffCapabilityState(
                        Instance='on',
                        Value=False,
                    ),
                    AnalyticsType='devices.capabilities.on_off',
                    Retrievable=True,
                    Reportable=True,
                ),
                TCapability(
                    Type=ECapabilityType.ToggleCapabilityType,
                    ToggleCapabilityParameters=TCapability.TToggleCapabilityParameters(),
                    ToggleCapabilityState=TCapability.TToggleCapabilityState(
                        Instance='pause',
                        Value=False,
                    ),
                    AnalyticsType='devices.capabilities.toggle',
                    Retrievable=True,
                    Reportable=True,
                )
            ]
        )
    ]
)
