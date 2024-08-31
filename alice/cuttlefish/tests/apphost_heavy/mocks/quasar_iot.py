import json
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.location.room_pb2 import TUserRoom


class QuasarIot:
    @classmethod
    def process(cls, name, request):
        content = TIoTUserInfo(
            Rooms=[
                TUserRoom(Id="1", Name="kitchen"),
                TUserRoom(Id="2", Name="bedroom"),
                TUserRoom(Id="3", Name="toilet"),
            ],
            CurrentHouseholdId="home sweet home",
            RawUserInfo=json.dumps(
                {
                    "payload": {
                        "rooms": [
                            {"name": "kitchen", "id": "1"},
                            {"name": "bedroom", "id": "2"},
                            {"name": "toilet", "id": "3"},
                        ]
                    }
                }
            ),
        )

        request.add_item(item_type="iot_user_info", item=content)
