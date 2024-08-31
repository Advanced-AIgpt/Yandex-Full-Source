from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.location.group_pb2 import TUserGroup
from alice.protos.data.location.room_pb2 import TUserRoom


config_for_quality_test = TIoTUserInfo(
    Rooms=[
        TUserRoom(Id='kitchen', Name='кухня'),
        TUserRoom(Id='bedroom', Name='спальня'),
        TUserRoom(Id='hall', Name='зал'),
        TUserRoom(Id='living-room', Name='гостиная'),
        TUserRoom(Id='kids-room', Name='детская'),
    ],
    Groups=[
        TUserGroup(Id='minis-group', Name='миники'),
        TUserGroup(Id='stations-group', Name='станции'),
        TUserGroup(Id='window-group', Name='окно'),
        TUserGroup(Id='wardrobe-group', Name='шкаф'),
        TUserGroup(Id='group-1', Name='группа 1'),
    ],
    Devices=[
        TIoTUserInfo.TDevice(
            Id='station-in-the-kitchen',
            Name='станция',
            QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(
                DeviceId='station-in-the-kitchen'
            ),
            RoomId='kitchen',
            GroupIds=['minis_group']
        )
    ]
)
