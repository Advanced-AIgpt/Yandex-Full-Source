package model

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/protos/data/location"
)

type Room struct {
	ID          string       `json:"id"`
	Name        string       `json:"name"`
	Devices     []string     `json:"devices,omitempty"`
	HouseholdID string       `json:"household_id,omitempty"`
	SharingInfo *SharingInfo `json:"sharing_info,omitempty"`
}

//means there is no devices in room
func (r *Room) IsEmpty() bool {
	return len(r.Devices) == 0
}

func (r *Room) GetName() string {
	if r == nil {
		return ""
	}
	return r.Name
}

func (r *Room) AssertName() error {
	return validName(r.Name, RoomNameLength)
}

func (r *Room) ValidateName(rooms []Room) error {
	if err := r.AssertName(); err != nil {
		return err
	}
	for _, room := range rooms {
		if room.ID == r.ID {
			continue
		}
		if tools.IsAlphanumericEqual(room.Name, r.Name) {
			return &NameIsAlreadyTakenError{}
		}
	}
	return nil
}

func (r Room) ToProto() *protos.Room {
	return &protos.Room{
		Name:    r.Name,
		Id:      r.ID,
		Devices: r.Devices,
	}
}

func (r *Room) FromProto(pr *protos.Room) {
	r.Name = pr.Name
	r.ID = pr.Id
	r.Devices = append(r.Devices, pr.Devices...)
}

func (r *Room) ToUserInfoProto() *location.TUserRoom {
	return &location.TUserRoom{
		Id:          r.ID,
		Name:        r.Name,
		HouseholdId: r.HouseholdID,
		SharingInfo: r.SharingInfo.ToUserInfoProto(),
	}
}

type Rooms []Room

func (rs Rooms) ToProto() []*protos.Room {
	result := make([]*protos.Room, 0, len(rs))
	for _, r := range rs {
		result = append(result, r.ToProto())
	}
	return result
}

func (rs Rooms) GetRoomByID(id string) (Room, bool) {
	for _, r := range rs {
		if r.ID == id {
			return r, true
		}
	}
	return Room{}, false
}

func (rs Rooms) ToMap() map[string]Room {
	result := make(map[string]Room)
	for _, room := range rs {
		result[room.ID] = room
	}
	return result
}

func (rs Rooms) SetSharingInfo(sharingInfos SharingInfos) {
	sharingInfoMap := sharingInfos.ToHouseholdMap()
	for i := range rs {
		sharingInfo, ok := sharingInfoMap[rs[i].HouseholdID]
		if !ok {
			continue
		}
		rs[i].SharingInfo = &sharingInfo
	}
}

func (rs Rooms) FilterByHouseholdIDs(householdIDs []string) Rooms {
	householdMap := make(map[string]bool, len(householdIDs))
	for _, householdID := range householdIDs {
		householdMap[householdID] = true
	}
	result := make(Rooms, 0, len(rs))
	for _, room := range rs {
		if householdMap[room.HouseholdID] {
			result = append(result, room)
		}
	}
	return result
}
