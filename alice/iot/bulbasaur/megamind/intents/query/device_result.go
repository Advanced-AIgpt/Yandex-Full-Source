package query

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

// Carefully stolen from megamind package

type DeviceCapabilityQueryResult struct {
	ID         string
	Name       string
	DeviceType model.DeviceType
	Room       *model.Room
	Capability model.ICapability
}

func NewDeviceCapabilityQueryResult(
	id string,
	name string,
	deviceType model.DeviceType,
	room *model.Room,
	capability model.ICapability,
) DeviceCapabilityQueryResult {
	return DeviceCapabilityQueryResult{id, name, deviceType, room, capability}
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

func NewDevicePropertyQueryResult(
	id string,
	name string,
	deviceType model.DeviceType,
	room *model.Room,
	property model.IProperty,
) DevicePropertyQueryResult {
	return DevicePropertyQueryResult{id, name, deviceType, room, property}
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
		if results[i-1].Property.Type() != model.FloatPropertyType ||
			results[i-1].Property.Type() != results[i].Property.Type() ||
			results[i-1].Property.Instance() != results[i].Property.Instance() {
			continue
		}

		previousUnit := results[i-1].Property.Parameters().(model.FloatPropertyParameters).Unit
		currentUnit := results[i].Property.Parameters().(model.FloatPropertyParameters).Unit
		if previousUnit != currentUnit {
			return false
		}
	}
	return true
}

func (results DevicePropertyQueryResults) GroupByID() map[string]DevicePropertyQueryResults {
	m := make(map[string]DevicePropertyQueryResults)
	for _, result := range results {
		m[result.ID] = append(m[result.ID], result)
	}
	return m
}

func (results DevicePropertyQueryResults) filterByInstance(instance model.PropertyInstance) DevicePropertyQueryResults {
	filtered := make(DevicePropertyQueryResults, 0, len(results))
	for _, result := range results {
		if result.Property.Instance() == string(instance) {
			filtered = append(filtered, result)
		}
	}
	return filtered
}

// filterByDeviceTypeForClimateInstances filters out some devices in cases described in
// https://st.yandex-team.ru/IOT-720#5fc806033ca599731bf6465b
func (results DevicePropertyQueryResults) filterByDeviceTypeForClimateInstances(devicePropertiesResults DevicePropertyQueryResults, instance model.PropertyInstance) DevicePropertyQueryResults {
	switch instance {
	case model.TemperaturePropertyInstance, model.HumidityPropertyInstance, model.CO2LevelPropertyInstance, model.PressurePropertyInstance:
		deviceTypeResultsMap := devicePropertiesResults.GroupByDeviceType()

		if deviceTypeResults, ok := deviceTypeResultsMap[model.SensorDeviceType]; ok {
			return deviceTypeResults
		}
		if deviceTypeResults, ok := deviceTypeResultsMap[model.AcDeviceType]; ok {
			return deviceTypeResults
		}
		if deviceTypeResults, ok := deviceTypeResultsMap[model.HumidifierDeviceType]; ok {
			return deviceTypeResults
		}
		if deviceTypeResults, ok := deviceTypeResultsMap[model.PurifierDeviceType]; ok {
			return deviceTypeResults
		}
		if deviceTypeResults, ok := deviceTypeResultsMap[model.OtherDeviceType]; ok {
			return deviceTypeResults
		}
	}
	return devicePropertiesResults
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
