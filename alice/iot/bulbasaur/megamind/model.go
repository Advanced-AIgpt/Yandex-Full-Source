package megamind

import (
	"context"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type SemanticFrame *common.TSemanticFrame

type QueryResult struct {
	Devices      []model.Device
	DeviceStates model.DeviceStatusMap
}

type DeviceCapabilityQueryResult struct {
	ID         string
	Name       string
	DeviceType model.DeviceType
	Room       *model.Room
	Capability model.ICapability
}

type DeviceCapabilityQueryResults []DeviceCapabilityQueryResult

func (results DeviceCapabilityQueryResults) Capabilities() model.Capabilities {
	capabilities := make(model.Capabilities, 0, len(results))
	for _, r := range results {
		capabilities = append(capabilities, r.Capability)
	}
	return capabilities
}

func (results DeviceCapabilityQueryResults) Rooms() model.Rooms {
	roomsMap := make(map[string]bool)
	r := make([]model.Room, 0)
	for _, d := range results {
		if d.Room != nil && !roomsMap[d.Room.ID] {
			r = append(r, *d.Room)
			roomsMap[d.Room.ID] = true
		}
	}
	return r
}

func (results DeviceCapabilityQueryResults) GroupByDeviceType() map[model.DeviceType]DeviceCapabilityQueryResults {
	m := make(map[model.DeviceType]DeviceCapabilityQueryResults)
	for _, deviceQueryResult := range results {
		m[deviceQueryResult.DeviceType] = append(m[deviceQueryResult.DeviceType], deviceQueryResult)
	}
	return m
}

type DeviceCapabilityQueryResultsSorting DeviceCapabilityQueryResults

func (results DeviceCapabilityQueryResultsSorting) Len() int {
	return len(results)
}

func (results DeviceCapabilityQueryResultsSorting) Less(i, j int) bool {
	iName, jName := strings.ToLower(results[i].Name), strings.ToLower(results[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return results[i].ID < results[j].ID
	}
}

func (results DeviceCapabilityQueryResultsSorting) Swap(i, j int) {
	results[i], results[j] = results[j], results[i]
}

type DevicePropertyQueryResult struct {
	ID         string
	Name       string
	DeviceType model.DeviceType
	Room       *model.Room
	Property   model.IProperty
}

type DevicePropertyQueryResults []DevicePropertyQueryResult

func (results DevicePropertyQueryResults) GroupByDeviceType() map[model.DeviceType]DevicePropertyQueryResults {
	m := make(map[model.DeviceType]DevicePropertyQueryResults)
	for _, deviceQueryResult := range results {
		m[deviceQueryResult.DeviceType] = append(m[deviceQueryResult.DeviceType], deviceQueryResult)
	}
	return m
}

func (results DevicePropertyQueryResults) Properties() model.Properties {
	properties := make(model.Properties, 0, len(results))
	for _, r := range results {
		properties = append(properties, r.Property)
	}
	return properties
}

func (results DevicePropertyQueryResults) Rooms() model.Rooms {
	roomsMap := make(map[string]bool)
	r := make([]model.Room, 0)
	for _, d := range results {
		if d.Room != nil && !roomsMap[d.Room.ID] {
			r = append(r, *d.Room)
			roomsMap[d.Room.ID] = true
		}
	}
	return r
}

func (results DevicePropertyQueryResults) CanBeAveraged() bool {
	for i := 1; i < len(results); i++ {
		if results[i-1].Property.Type() == results[i].Property.Type() {
			if results[i-1].Property.Type() == model.FloatPropertyType {
				previousUnit := results[i-1].Property.Parameters().(model.FloatPropertyParameters).Unit
				currentUnit := results[i].Property.Parameters().(model.FloatPropertyParameters).Unit
				if previousUnit == currentUnit {
					continue
				}
			}
		}
		return false
	}
	return true
}

type DevicePropertyQueryResultsSorting DevicePropertyQueryResults

func (results DevicePropertyQueryResultsSorting) Len() int {
	return len(results)
}

func (results DevicePropertyQueryResultsSorting) Less(i, j int) bool {
	iName, jName := strings.ToLower(results[i].Name), strings.ToLower(results[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return results[i].ID < results[j].ID
	}
}

func (results DevicePropertyQueryResultsSorting) Swap(i, j int) {
	results[i], results[j] = results[j], results[i]
}

func newOrigin(ctx context.Context, clientInfo libmegamind.ClientInfo, user model.User) model.Origin {
	if clientInfo.IsSmartSpeaker() {
		return model.NewOrigin(ctx, model.SpeakerSurfaceParameters{ID: clientInfo.DeviceID}, user)
	}
	return model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
}
