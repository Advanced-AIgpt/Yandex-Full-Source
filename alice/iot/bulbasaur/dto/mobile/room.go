package mobile

import (
	"context"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type RoomInfoView struct {
	ID      string           `json:"id"`
	Name    string           `json:"name"`
	Devices []DeviceInfoView `json:"devices"`
}

type UserRoomView struct {
	ID                  string           `json:"id"`
	Name                string           `json:"name"`
	ValidationError     *string          `json:"validation_error,omitempty"`
	NameValidationError *model.ErrorCode `json:"name_validation_error,omitempty"`
	Active              *bool            `json:"active,omitempty"`
	devices             []string
}

func (urv *UserRoomView) FromRoom(room model.Room) {
	urv.ID = room.ID
	urv.Name = room.Name
	urv.devices = room.Devices
	if err := room.AssertName(); err != nil {
		validationFailureMessage := model.RenameToRussianErrorMessage
		urv.ValidationError = &validationFailureMessage
		urv.NameValidationError = model.EC(nameValidationErrorToErrorCode(err))
	}
}

type UserRoomsView struct {
	Status    string         `json:"status"`
	RequestID string         `json:"request_id"`
	Rooms     []UserRoomView `json:"rooms"`
}

func (urv *UserRoomsView) FromRooms(rooms []model.Room) {
	userRooms := make([]UserRoomView, 0, len(rooms))
	for _, room := range rooms {
		userRoom := UserRoomView{}
		userRoom.FromRoom(room)
		userRooms = append(userRooms, userRoom)
	}
	sort.Sort(userRoomViewByName(userRooms))
	urv.Rooms = userRooms
}

func (urv *UserRoomsView) MarkDeviceRoom(device model.Device) {
	for i, room := range urv.Rooms {
		active := false
		if tools.Contains(device.ID, room.devices) {
			active = true
		}
		urv.Rooms[i].Active = &active
	}
}

type RoomEditView struct {
	Status              string               `json:"status"`
	RequestID           string               `json:"request_id"`
	ID                  string               `json:"id"`
	Name                string               `json:"name"`
	NameValidationError *model.ErrorCode     `json:"name_validation_error,omitempty"`
	ValidationError     *string              `json:"validation_error,omitempty"`
	Suggests            []string             `json:"suggests"`
	Devices             []DeviceRoomEditView `json:"devices"`
}

func (rev *RoomEditView) From(ctx context.Context, room model.Room, roomDevices model.Devices, stereopairs model.Stereopairs) {
	rev.ID = room.ID
	rev.Name = room.Name

	if err := room.AssertName(); err != nil {
		validationFailureMessage := model.RenameToRussianErrorMessage
		rev.ValidationError = &validationFailureMessage
		rev.NameValidationError = model.EC(nameValidationErrorToErrorCode(err))
	}

	rev.Suggests = suggestions.RoomNames

	roomDevicesMap := roomDevices.ToMap()
	rev.Devices = make([]DeviceRoomEditView, 0, len(roomDevices))
	for _, deviceID := range room.Devices {
		if device, exist := roomDevicesMap[deviceID]; exist {
			var view DeviceRoomEditView
			switch stereopairs.GetDeviceRole(deviceID) {
			case model.LeaderRole:
				stereopair, _ := stereopairs.GetByDeviceID(deviceID)
				view.FromStereopair(ctx, stereopair)
			case model.FollowerRole:
				// skip follower device
				continue
			default:
				view.FromDevice(device)
			}
			rev.Devices = append(rev.Devices, view)
		}
	}
	sort.Sort(DeviceRoomEditViewSorting(rev.Devices))
}

type RoomCreateView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Suggests  []string `json:"suggests"`
}

func (rcv *RoomCreateView) FillSuggests() {
	rcv.Suggests = suggestions.RoomNames
}

type RoomName string

type RoomCreateRequest struct {
	Name        RoomName `json:"name"`
	Devices     []string `json:"devices"`
	HouseholdID *string  `json:"household_id,omitempty"`
}

func (req RoomCreateRequest) ToRoom() model.Room {
	var householdID string
	if req.HouseholdID != nil {
		householdID = *req.HouseholdID
	}
	return model.Room{
		Name:        string(req.Name),
		Devices:     req.Devices,
		HouseholdID: householdID,
	}
}

type RoomUpdateRequest struct {
	Name    *RoomName `json:"name"`
	Devices []string  `json:"devices"`
}

func (req RoomUpdateRequest) ApplyOnRoom(room model.Room) model.Room {
	res := room
	if req.Name != nil {
		res.Name = string(*req.Name)
	}
	if req.Devices != nil {
		res.Devices = req.Devices
	}
	return res
}

func (req RoomUpdateRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	if req.Name != nil {
		if _, e := req.Name.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				verrs = append(verrs, ves...)
			} else {
				verrs = append(verrs, e)
			}
		}
	}
	if len(verrs) != 0 {
		return false, verrs
	}
	return false, nil
}

type RoomRenameRequest struct {
	Name RoomName `json:"name"`
}

func (rn RoomName) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	room := model.Room{Name: string(rn)}
	if err := room.AssertName(); err != nil {
		verrs = append(verrs, err)
	}

	if len(verrs) > 0 {
		return false, verrs
	}
	return false, nil
}

type DeviceRoomEditView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
}

func (view *DeviceRoomEditView) FromDevice(device model.Device) {
	view.ID = device.ID
	view.Name = device.Name
	view.Type = device.Type
	view.ItemType = DeviceItemInfoViewType
	view.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		view.QuasarInfo = &quasarInfo
	}
}

func (view *DeviceRoomEditView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	view.FromDevice(stereopair.GetLeaderDevice())
	view.Name = stereopair.Name
	view.ItemType = StereopairItemInfoViewType
	view.Stereopair = &StereopairView{}
	view.Stereopair.From(ctx, stereopair)
}

type RoomAvailableDevicesResponse struct {
	Status     string                              `json:"status"`
	RequestID  string                              `json:"request_id"`
	Households []HouseholdRoomAvailableDevicesView `json:"households"`
}

func (resp *RoomAvailableDevicesResponse) From(ctx context.Context, currentHouseholdID string, userDevices model.Devices, userHouseholds []model.Household, roomID string, stereopairs model.Stereopairs) {
	householdDevicesMap := userDevices.GroupByHousehold()
	resp.Households = make([]HouseholdRoomAvailableDevicesView, 0, len(userHouseholds))
	for _, household := range userHouseholds {
		var view HouseholdRoomAvailableDevicesView
		view.From(ctx, currentHouseholdID, household, householdDevicesMap[household.ID], roomID, stereopairs)
		resp.Households = append(resp.Households, view)
	}
	sort.Sort(HouseholdRoomAvailableDevicesViewSorting(resp.Households))
}

type HouseholdRoomAvailableDevicesView struct {
	ID                  string                       `json:"id"`
	Name                string                       `json:"name"`
	IsCurrent           bool                         `json:"is_current"`
	Rooms               []RoomAvailableDevicesView   `json:"rooms"`
	UnconfiguredDevices []DeviceAvailableForRoomView `json:"unconfigured_devices"`
}

func (view *HouseholdRoomAvailableDevicesView) From(ctx context.Context, currentHouseholdID string, household model.Household, householdDevices model.Devices, roomID string, stereopairs model.Stereopairs) {
	view.ID = household.ID
	view.Name = household.Name
	if currentHouseholdID == household.ID {
		view.IsCurrent = true
	}
	view.UnconfiguredDevices = make([]DeviceAvailableForRoomView, 0, len(householdDevices))
	roomsMap := make(map[string]RoomAvailableDevicesView)
	for _, d := range householdDevices {
		var deviceView DeviceAvailableForRoomView
		switch stereopairs.GetDeviceRole(d.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(d.ID)
			deviceView.FromStereopair(ctx, stereopair, roomID)
		case model.FollowerRole:
			// skip
			continue
		default:
			deviceView.From(d, roomID)
		}

		if d.Room == nil {
			view.UnconfiguredDevices = append(view.UnconfiguredDevices, deviceView)
			continue
		}
		roomView, exist := roomsMap[d.Room.ID]
		if !exist {
			roomView = RoomAvailableDevicesView{}
			roomView.FromRoom(*d.Room)
		}
		roomView.Devices = append(roomView.Devices, deviceView)
		roomsMap[roomView.ID] = roomView
	}
	view.Rooms = make([]RoomAvailableDevicesView, 0, len(roomsMap))
	for _, room := range roomsMap {
		sort.Sort(DeviceAvailableForRoomViewSorting(room.Devices))
		view.Rooms = append(view.Rooms, room)
	}
	sort.Sort(RoomAvailableDevicesViewSorting(view.Rooms))
	sort.Sort(DeviceAvailableForRoomViewSorting(view.UnconfiguredDevices))
}

type RoomAvailableDevicesView struct {
	ID      string                       `json:"id"`
	Name    string                       `json:"name"`
	Devices []DeviceAvailableForRoomView `json:"devices"`
}

func (view *RoomAvailableDevicesView) FromRoom(room model.Room) {
	view.ID = room.ID
	view.Name = room.Name
}

type DeviceAvailableForRoomView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	SkillID    string           `json:"skill_id"`
	IsSelected *bool            `json:"is_selected,omitempty"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
}

func (view *DeviceAvailableForRoomView) From(device model.Device, roomID string) {
	view.ID = device.ID
	view.Name = device.Name
	view.Type = device.Type
	view.SkillID = device.SkillID
	view.ItemType = DeviceItemInfoViewType
	view.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.RoomID() == roomID {
		view.IsSelected = ptr.Bool(true)
	}
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		view.QuasarInfo = &quasarInfo
	}
}
func (view *DeviceAvailableForRoomView) FromStereopair(ctx context.Context, stereopair model.Stereopair, roomID string) {
	view.From(stereopair.GetLeaderDevice(), roomID)
	view.ItemType = StereopairItemInfoViewType
	view.Name = stereopair.Name
	view.Stereopair = &StereopairView{}
	view.Stereopair.From(ctx, stereopair)
}

type RoomDiscoveryView struct {
	ID                      string          `json:"id"`
	Name                    string          `json:"name"`
	NameValidationErrorCode model.ErrorCode `json:"name_validation_error_code,omitempty"`
}

func (v *RoomDiscoveryView) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
	if err := room.AssertName(); err != nil {
		v.NameValidationErrorCode = nameValidationErrorToErrorCode(err)
	}
}
