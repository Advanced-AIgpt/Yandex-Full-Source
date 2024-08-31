from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.device.info_pb2 import EUserDeviceType
from alice.protos.data.location.room_pb2 import TUserRoom

TCapability = TIoTUserInfo.TCapability
TProperty = TIoTUserInfo.TProperty
ECapabilityType = TCapability.ECapabilityType

curtain = TIoTUserInfo(
    RawUserInfo='{}',
    Scenarios=[],
    Colors=[],
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
            Id='evo-test-curtain-id-1',
            ExternalId='evo-test-curtain-external-id-1',
            Name='Шторы',
            RoomId='room-test-id-1',
            Aliases=[],
            SkillId='QUALITY',
            Type=EUserDeviceType.CurtainDeviceType,
            OriginalType=EUserDeviceType.CurtainDeviceType,
            AnalyticsType='devices.types.openable.curtain',
            HouseholdId='evo-test-household-id-1',
            Capabilities=[
                TCapability(
                    Type=ECapabilityType.OnOffCapabilityType,
                    OnOffCapabilityParameters=TCapability.TOnOffCapabilityParameters(),
                    OnOffCapabilityState=TCapability.TOnOffCapabilityState(),
                    AnalyticsType='devices.capabilities.on_off',
                    Retrievable=True,
                    Reportable=True,
                ),
                TCapability(
                    Type=ECapabilityType.RangeCapabilityType,
                    RangeCapabilityParameters=TCapability.TRangeCapabilityParameters(
                        Range=TCapability.TRangeCapabilityParameters.TRange(
                            Min=1,
                            Max=100,
                            Precision=1,
                        ),
                        RandomAccess=True,
                        Unit='unit.percent',
                        Instance='open',
                        Looped=False,
                    ),
                )
            ]
        ),
    ]
)

