package libmegamind

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type IotUserInfo struct {
	IotUserInfo *common.TIoTUserInfo
}

func (shi IotUserInfo) GetRoomNameBySpeakerQuasarID(speakerQuasarID string) string {
	var speakerRoomID string
	for _, device := range shi.IotUserInfo.GetDevices() {
		if quasarInfo := device.GetQuasarInfo(); quasarInfo != nil {
			if quasarInfo.GetDeviceId() == speakerQuasarID {
				speakerRoomID = device.GetRoomId()
				break
			}
		}
	}
	if len(speakerRoomID) == 0 {
		return ""
	}
	for _, room := range shi.IotUserInfo.GetRooms() {
		if room.GetId() == speakerRoomID {
			return room.Name
		}
	}
	return ""
}
