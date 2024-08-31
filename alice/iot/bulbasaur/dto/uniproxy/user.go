package uniproxy

import (
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/property"
	"a.yandex-team.ru/yt/go/yson"
)

type UserInfoResponse struct {
	Status    string       `json:"status"`
	RequestID string       `json:"request_id"`
	Payload   UserInfoView `json:"payload"`
}

func NewUserInfoResponse(rID string) UserInfoResponse {
	uiv := UserInfoResponse{Status: "ok", RequestID: rID}
	uiv.Payload.Scenarios = []ScenarioUserInfoView{}
	uiv.Payload.Colors = []ColorUserInfoView{}
	uiv.Payload.Devices = []DeviceUserInfoView{}
	uiv.Payload.Rooms = []RoomUserInfoView{}
	uiv.Payload.Groups = []GroupUserInfoView{}
	return uiv
}

type UserInfoView struct {
	Devices   []DeviceUserInfoView   `json:"devices" yson:"devices"`
	Scenarios []ScenarioUserInfoView `json:"scenarios" yson:"scenarios"`
	Colors    []ColorUserInfoView    `json:"colors" yson:"colors"` // legacy object of plain UserInfo config
	Rooms     []RoomUserInfoView     `json:"rooms" yson:"rooms"`
	Groups    []GroupUserInfoView    `json:"groups" yson:"groups"`
}

func NewEmptyUserInfoView() UserInfoView {
	return UserInfoView{}
}

func NewUserInfoView(userInfo model.UserInfo) UserInfoView {
	userInfoView := UserInfoView{}
	userInfoView.PopulateDevices(userInfo.Devices)
	userInfoView.PopulateGroups(userInfo.Groups)
	userInfoView.PopulateRooms(userInfo.Rooms)
	userInfoView.PopulateScenarios(userInfo.Scenarios)
	return userInfoView
}

func (u *UserInfoView) PopulateDevices(devices []model.Device) {
	u.Devices = make([]DeviceUserInfoView, 0, len(devices))
	capabilitySet := make(map[CapabilityUserInfoKey]capabilityKeyInfo)
	colorSet := make(map[ColorUserInfoView]bool)

	//fill devices
	for _, device := range devices {
		duiv := DeviceUserInfoView{
			ID:            device.ID,
			ExternalID:    device.ExternalID,
			Name:          device.Name,
			Aliases:       make([]string, 0, len(device.Aliases)),
			Type:          device.Type,
			OriginalType:  device.OriginalType,
			IconURL:       device.Type.IconURL(model.RawIconFormat),
			AnalyticsType: getAnalyticsType(device.Type),
			Groups:        make([]GroupUserInfoView, 0, len(device.Groups)),
			Capabilities:  make([]CapabilityUserInfoView, 0, len(device.Capabilities)), // IOT-273
			Properties:    make([]PropertyUserInfoView, 0, len(device.Properties)),
			Created:       device.Created,
		}

		if len(device.Aliases) != 0 {
			duiv.Aliases = device.Aliases
		}

		if device.Room != nil {
			duiv.RoomID = device.Room.ID
		}

		for _, g := range device.Groups {
			guiv := GroupUserInfoView{
				ID:      g.ID,
				Name:    g.Name,
				Aliases: g.Aliases,
				Type:    g.Type,
			}
			duiv.Groups = append(duiv.Groups, guiv)
		}

		for _, c := range device.Capabilities {
			cuiv := CapabilityUserInfoView{
				Type:        c.Type(),
				Instance:    c.Instance(),
				Retrievable: c.Retrievable(),
				Parameters:  c.Parameters(),
				State:       c.State(),
			}

			switch c.Type() {
			case model.ColorSettingCapabilityType:
				//add color instance explicitly
				cuiv.Instance = ColorCapabilityInstance
				cuiv.AnalyticsName = getAnalyticsName(cuiv.ToKey())
				capabilitySet[cuiv.ToKey()] = capabilityKeyInfo{}
				duiv.Capabilities = append(duiv.Capabilities, cuiv)

				//add color-temperature instance (if any) explicitly
				capabilityParameters := c.Parameters().(model.ColorSettingCapabilityParameters)
				if capabilityParameters.TemperatureK != nil {
					cuivTK := CapabilityUserInfoView{
						Type:        c.Type(),
						Instance:    string(model.TemperatureKCapabilityInstance),
						Retrievable: c.Retrievable(),
						Parameters:  c.Parameters(),
						State:       c.State(),
					}
					cuivTK.AnalyticsName = getAnalyticsName(cuivTK.ToKey())
					capabilitySet[cuivTK.ToKey()] = capabilityKeyInfo{}
					duiv.Capabilities = append(duiv.Capabilities, cuivTK)
				}
				for _, color := range capabilityParameters.GetAvailableColors() {
					cuiv := ColorUserInfoView{ID: color.ID, Name: color.Name}
					colorSet[cuiv] = true

					// add aliases to colors
					for _, colorAlias := range model.ColorIDToAdditionalAliases[color.ID] {
						cuiv := ColorUserInfoView{ID: color.ID, Name: colorAlias}
						colorSet[cuiv] = true
					}
				}
			case model.ModeCapabilityType:
				capabilityParameters := c.Parameters().(model.ModeCapabilityParameters)
				cuiv.Values = make([]string, 0, len(capabilityParameters.Modes))
				modeSetInfo, exist := capabilitySet[cuiv.ToKey()]
				if !exist {
					modeSetInfo.modeMap = make(map[model.ModeValue]struct{}, len(capabilityParameters.Modes))
				}
				for _, mode := range capabilityParameters.Modes {
					cuiv.Values = append(cuiv.Values, string(mode.Value))
					modeSetInfo.modeMap[mode.Value] = struct{}{}
				}
				cuiv.AnalyticsName = getAnalyticsName(cuiv.ToKey())
				capabilitySet[cuiv.ToKey()] = modeSetInfo
				duiv.Capabilities = append(duiv.Capabilities, cuiv)
			case model.CustomButtonCapabilityType:
				capabilityParameters := c.Parameters().(model.CustomButtonCapabilityParameters)
				cuiv.AnalyticsName = getAnalyticsName(cuiv.ToKey())
				cuiv.InstanceNames = capabilityParameters.InstanceNames //TODO: temporary fix to satisfy Jan/igor
				instanceNamesInfo, exist := capabilitySet[cuiv.ToKey()]
				if !exist {
					instanceNamesInfo.instanceNamesMap = make(map[string]struct{}, len(capabilityParameters.InstanceNames))
				}
				for _, instanceName := range capabilityParameters.InstanceNames {
					instanceNamesInfo.instanceNamesMap[instanceName] = struct{}{}
				}
				capabilitySet[cuiv.ToKey()] = instanceNamesInfo
				duiv.Capabilities = append(duiv.Capabilities, cuiv)
			case model.QuasarServerActionCapabilityType, model.QuasarCapabilityType, model.VideoStreamCapabilityType:
			default:
				cuiv.AnalyticsName = getAnalyticsName(cuiv.ToKey())
				capabilitySet[cuiv.ToKey()] = capabilityKeyInfo{}
				duiv.Capabilities = append(duiv.Capabilities, cuiv)
			}
		}

		for _, p := range device.Properties {
			if p.Type() != model.FloatPropertyType {
				continue
			}

			puiv := PropertyUserInfoView{
				Type:        p.Type(),
				Instance:    p.Instance(),
				Retrievable: p.Retrievable(),
				Parameters:  p.Parameters(),
				State:       p.State(),
			}
			puiv.AnalyticsName = getPropertyAnalyticsName(puiv.ToKey())
			duiv.Properties = append(duiv.Properties, puiv)
		}

		if device.IsQuasarDevice() || device.IsVirtualQuasarDevice() {
			if quasarData, err := device.QuasarCustomData(); err == nil {
				duiv.QuasarInfo = &QuasarInfo{
					DeviceID: quasarData.DeviceID,
					Platform: quasarData.Platform,
				}
			}
		}

		u.Devices = append(u.Devices, duiv)
	}

	//fill colors
	u.Colors = make([]ColorUserInfoView, 0, len(colorSet))
	for color := range colorSet {
		u.Colors = append(u.Colors, color)
	}
}

type capabilityKeyInfo struct {
	modeMap          map[model.ModeValue]struct{}
	instanceNamesMap map[string]struct{}
}

func (u *UserInfoView) PopulateRooms(rooms []model.Room) {
	roomSet := make(map[string]model.Room)

	for _, r := range rooms {
		roomSet[r.ID] = r
	}

	u.Rooms = make([]RoomUserInfoView, 0, len(roomSet))
	for _, v := range roomSet {
		u.Rooms = append(u.Rooms, RoomUserInfoView{
			ID:   v.ID,
			Name: v.Name,
		})
	}
}

func (u *UserInfoView) PopulateGroups(groups []model.Group) {
	groupSet := make(map[string]model.Group)

	for _, g := range groups {
		groupSet[g.ID] = g
	}

	u.Groups = make([]GroupUserInfoView, 0, len(groupSet))
	for _, v := range groupSet {
		u.Groups = append(u.Groups, GroupUserInfoView{
			ID:      v.ID,
			Name:    v.Name,
			Aliases: v.Aliases,
			Type:    v.Type,
		})
	}
}

func (u *UserInfoView) PopulateScenarios(scenarios []model.Scenario) {
	u.Scenarios = make([]ScenarioUserInfoView, 0, len(scenarios))
	for _, s := range scenarios {
		if !s.IsActive {
			continue
		}
		suiv := ScenarioUserInfoView{
			ID:      s.ID,
			Name:    s.Name,
			Devices: s.Devices,
			Icon:    s.Icon,
		}
		s.Triggers.Normalize()

		//suiv.Triggers = s.Triggers //TODO: temporary commented to fix megamind, see IOT-701
		suiv.Triggers = s.Triggers.Filter(func(t model.ScenarioTrigger) bool {
			return t.Type() != model.TimetableScenarioTriggerType && t.Type() != model.PropertyScenarioTriggerType
		})
		suiv.RequestedSpeakerCapabilities = make([]model.ScenarioCapability, 0, len(s.RequestedSpeakerCapabilities))
		suiv.RequestedSpeakerCapabilities = append(suiv.RequestedSpeakerCapabilities, s.RequestedSpeakerCapabilities...)
		u.Scenarios = append(u.Scenarios, suiv)
	}
}

type DeviceUserInfoView struct {
	ID            string                   `json:"id" yson:"id"`
	ExternalID    string                   `json:"external_id" yson:"external_id"` //needed by passport https://st.yandex-team.ru/IOT-432
	Name          string                   `json:"name" yson:"name"`
	Aliases       []string                 `json:"aliases" yson:"aliases"`
	RoomID        string                   `json:"room_id" yson:"room_id"`
	Groups        []GroupUserInfoView      `json:"groups" yson:"groups"`
	Capabilities  []CapabilityUserInfoView `json:"capabilities" yson:"capabilities"`
	Properties    []PropertyUserInfoView   `json:"properties" yson:"properties"`
	Type          model.DeviceType         `json:"type" yson:"type"`
	OriginalType  model.DeviceType         `json:"original_type" yson:"original_type"`
	IconURL       string                   `json:"icon_url" yson:"icon_url"`
	AnalyticsType string                   `json:"analytics_type" yson:"analytics_type"`
	QuasarInfo    *QuasarInfo              `json:"quasar_info,omitempty" yson:"quasar_info"` //needed by alice_b2b https://a.yandex-team.ru/review/1071684
	Created       timestamp.PastTimestamp  `json:"created" yson:"created"`
}

func (duiv DeviceUserInfoView) ToDevice() model.Device {
	var d model.Device

	d.ID = duiv.ID
	d.ExternalID = duiv.ExternalID
	d.Name = duiv.Name

	if len(duiv.Aliases) != 0 {
		d.Aliases = duiv.Aliases
	}

	d.Type = duiv.Type
	d.OriginalType = duiv.OriginalType
	d.Room = &model.Room{ID: duiv.RoomID}

	d.Groups = make([]model.Group, 0, len(duiv.Groups))
	for _, g := range duiv.Groups {
		d.Groups = append(d.Groups, g.ToGroup())
	}

	cs := make(model.Capabilities, 0, len(duiv.Capabilities))
	var seenColorSettingCapability bool
	for _, cuiv := range duiv.Capabilities {
		if cuiv.Type == model.ColorSettingCapabilityType && seenColorSettingCapability { //skip duplicated ColorSettingCapability
			continue
		}
		c := cuiv.ToCapability()
		cs = append(cs, c)

		if cuiv.Type == model.ColorSettingCapabilityType {
			seenColorSettingCapability = true
		}
	}
	d.Capabilities = cs

	ps := make(model.Properties, 0, len(duiv.Properties))
	for _, puiv := range duiv.Properties {
		p := puiv.ToProperty()
		ps = append(ps, p)
	}
	d.Properties = ps

	return d
}

type ScenarioUserInfoView struct {
	ID                           string                     `json:"id" yson:"id"`
	Name                         model.ScenarioName         `json:"name" yson:"name"`
	Triggers                     []model.ScenarioTrigger    `json:"triggers" yson:"triggers"`
	Icon                         model.ScenarioIcon         `json:"icon" yson:"icon"`
	Devices                      []model.ScenarioDevice     `json:"devices" yson:"devices"`
	RequestedSpeakerCapabilities []model.ScenarioCapability `json:"requested_speaker_capabilities" yson:"requested_speaker_capabilities"`
}

func (suiv ScenarioUserInfoView) ToScenario() model.Scenario {
	var s model.Scenario
	s.ID = suiv.ID
	s.Name = suiv.Name
	s.Icon = suiv.Icon

	sc := make([]model.ScenarioDevice, 0, len(suiv.Devices))
	sc = append(sc, suiv.Devices...)
	s.Devices = sc

	srsc := make(model.ScenarioCapabilities, 0, len(suiv.RequestedSpeakerCapabilities))
	srsc = append(srsc, suiv.RequestedSpeakerCapabilities...)
	s.RequestedSpeakerCapabilities = srsc
	return s
}

func (suiv ScenarioUserInfoView) ToScenarioWithTriggersProto() *property.TIotProfile_TScenarioWithTriggers {
	triggers := make([]*property.TIotProfile_TScenarioTrigger, 0, len(suiv.Triggers))
	for _, t := range suiv.Triggers {
		var trigger *property.TIotProfile_TScenarioTrigger

		switch scenarioTrigger := t.(type) {
		case model.VoiceScenarioTrigger:
			trigger = &property.TIotProfile_TScenarioTrigger{
				TriggerType: property.TIotProfile_VoiceScenarioTriggerType,
				Value: &property.TIotProfile_TScenarioTrigger_VoiceTriggerPhrase{
					VoiceTriggerPhrase: scenarioTrigger.Phrase,
				},
			}
		case model.TimerScenarioTrigger:
			trigger = &property.TIotProfile_TScenarioTrigger{
				TriggerType: property.TIotProfile_TimerScenarioTriggerType,
				Value: &property.TIotProfile_TScenarioTrigger_TimerTriggerTimestamp{
					TimerTriggerTimestamp: scenarioTrigger.Time.AsTime().Format(time.RFC3339),
				},
			}
		default:
			continue
		}

		triggers = append(triggers, trigger)
	}

	return &property.TIotProfile_TScenarioWithTriggers{
		Id:       suiv.ID,
		Name:     string(suiv.Name),
		Triggers: triggers,
	}
}

type PropertyUserInfoView struct {
	Type          model.PropertyType        `json:"type" yson:"type"`
	Instance      string                    `json:"instance" yson:"instance"`
	AnalyticsName string                    `json:"analytics_name" yson:"analytics_name"`
	Retrievable   bool                      `json:"retrievable" yson:"retrievable"`
	Parameters    model.IPropertyParameters `json:"parameters" yson:"parameters"`
	State         model.IPropertyState      `json:"state" yson:"state"`
}

func (puiv *PropertyUserInfoView) UnmarshalYSON(b []byte) (err error) {
	cRaw := struct {
		Type        model.PropertyType `yson:"type"`
		Retrievable bool               `yson:"retrievable"`
		Parameters  yson.RawValue      `yson:"parameters"`
		State       yson.RawValue      `yson:"state"`
	}{}
	if err := yson.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	puiv.Retrievable = cRaw.Retrievable

	switch puiv.Type {
	case model.EventPropertyType:
		puiv.Type = model.EventPropertyType
		s := model.EventPropertyState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.EventPropertyState{}) {
			puiv.State = s
		}
		p := model.EventPropertyParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		puiv.Parameters = p

	case model.FloatPropertyType, "": //FIXME: empty `puiv.Type` means we don't know type, as type field in PropertyUserInfoView invented later
		puiv.Type = model.FloatPropertyType
		s := model.FloatPropertyState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.FloatPropertyState{}) {
			puiv.State = s
		}
		p := model.FloatPropertyParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		puiv.Parameters = p

	default:
		return fmt.Errorf("unknown property type: %s", cRaw.Type)
	}

	return nil
}

func (puiv *PropertyUserInfoView) ToProperty() model.IProperty {
	p := model.MakePropertyByType(puiv.Type)
	p.SetRetrievable(puiv.Retrievable)
	p.SetParameters(puiv.Parameters)
	if puiv.State != nil {
		p.SetState(puiv.State)
	}
	return p
}

func (puiv *PropertyUserInfoView) ToKey() PropertyUserInfoKey {
	return PropertyUserInfoKey{Type: puiv.Type, Instance: puiv.Instance}
}

type PropertyUserInfoKey struct {
	Type     model.PropertyType
	Instance string
}

type CapabilityUserInfoView struct {
	Type          model.CapabilityType        `json:"type" yson:"type"`
	Instance      string                      `json:"instance" yson:"instance"`
	AnalyticsName string                      `json:"analytics_name" yson:"analytics_name"`
	Values        []string                    `json:"values,omitempty" yson:"values,omitempty"`
	InstanceNames []string                    `json:"instance_names,omitempty" yson:"instance_names,omitempty"`
	Retrievable   bool                        `json:"retrievable" yson:"retrievable"`
	Parameters    model.ICapabilityParameters `json:"parameters" yson:"parameters"`
	State         model.ICapabilityState      `json:"state" yson:"state"`
}

func (cuiv *CapabilityUserInfoView) UnmarshalYSON(b []byte) (err error) {
	cRaw := struct {
		Type        model.CapabilityType `yson:"type"`
		Retrievable bool                 `yson:"retrievable"`
		Parameters  yson.RawValue        `yson:"parameters"`
		State       yson.RawValue        `yson:"state"`
	}{}
	if err := yson.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	cuiv.Type = cRaw.Type
	cuiv.Retrievable = cRaw.Retrievable

	switch cuiv.Type {
	case model.OnOffCapabilityType:
		s := model.OnOffCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.OnOffCapabilityState{}) {
			cuiv.State = s
		}
		p := model.OnOffCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.ColorSettingCapabilityType:
		s := model.ColorSettingCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.ColorSettingCapabilityState{}) {
			cuiv.State = s
		}
		p := model.ColorSettingCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.ModeCapabilityType:
		s := model.ModeCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.ModeCapabilityState{}) {
			cuiv.State = s
		}
		p := model.ModeCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.RangeCapabilityType:
		s := model.RangeCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.RangeCapabilityState{}) {
			cuiv.State = s
		}
		p := model.RangeCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.ToggleCapabilityType:
		s := model.ToggleCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.ToggleCapabilityState{}) {
			cuiv.State = s
		}
		p := model.ToggleCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.CustomButtonCapabilityType:
		s := model.CustomButtonCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if s != (model.CustomButtonCapabilityState{}) {
			cuiv.State = s
		}
		p := model.CustomButtonCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	case model.VideoStreamCapabilityType:
		s := model.VideoStreamCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		if !s.IsEmpty() {
			cuiv.State = s
		}
		p := model.VideoStreamCapabilityParameters{}
		if err := yson.Unmarshal(cRaw.Parameters, &p); err != nil {
			return err
		}
		cuiv.Parameters = p

	default:
		return fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}

	return nil
}

func (cuiv *CapabilityUserInfoView) ToCapability() model.ICapability {
	c := model.MakeCapabilityByType(cuiv.Type)
	c.SetRetrievable(cuiv.Retrievable)
	c.SetParameters(cuiv.Parameters)
	if cuiv.State != nil {
		c.SetState(cuiv.State)
	}
	return c
}

func (cuiv *CapabilityUserInfoView) ToKey() CapabilityUserInfoKey {
	return CapabilityUserInfoKey{Type: cuiv.Type, Instance: cuiv.Instance}
}

type CapabilityUserInfoKey struct {
	Type     model.CapabilityType
	Instance string
}

type ColorUserInfoView struct {
	ID   model.ColorID `json:"id" yson:"id"`
	Name string        `json:"name" yson:"name"`
}

type QuasarInfo struct {
	DeviceID string `json:"device_id"`
	Platform string `json:"platform"`
}

type RoomUserInfoView struct {
	ID   string `json:"id" yson:"id"`
	Name string `json:"name" yson:"name"`
}

func (ruiv RoomUserInfoView) ToRoom() model.Room {
	return model.Room{
		ID:   ruiv.ID,
		Name: ruiv.Name,
	}
}

type GroupUserInfoView struct {
	ID      string           `json:"id" yson:"id"`
	Name    string           `json:"name" yson:"name"`
	Aliases []string         `json:"aliases" yson:"aliases"`
	Type    model.DeviceType `json:"type" yson:"type"`
}

func (guiv GroupUserInfoView) ToGroup() model.Group {
	return model.Group{
		ID:   guiv.ID,
		Name: guiv.Name,
		Type: guiv.Type,
	}
}
