package model

import (
	"encoding/json"
	"fmt"
	"strings"
)

type FavoriteType string

type Favorites struct {
	Scenarios  Scenarios                 `json:"scenarios"`
	Devices    Devices                   `json:"devices"`
	Groups     Groups                    `json:"groups"`
	Properties FavoritesDeviceProperties `json:"properties"`
}

type FavoritesDeviceProperty struct {
	DeviceID string    `json:"device_id"`
	Property IProperty `json:"property"`
}

func (p *FavoritesDeviceProperty) FromProperty(deviceID string, property IProperty) {
	p.DeviceID = deviceID
	p.Property = property.Clone()
}

func (p *FavoritesDeviceProperty) DevicePropertyKey() DevicePropertyKey {
	return DevicePropertyKey{DeviceID: p.DeviceID, PropertyKey: p.Property.Key()}
}

func (p *FavoritesDeviceProperty) PropertyKey() string {
	return p.Property.Key()
}

type FavoritesDeviceProperties []FavoritesDeviceProperty

func (f FavoritesDeviceProperties) Contains(propertyKey string, deviceID string) bool {
	for _, p := range f {
		if p.DeviceID == deviceID && p.Property.Key() == propertyKey {
			return true
		}
	}
	return false
}

type DevicePropertyKey struct {
	DeviceID    string
	PropertyKey string
}

type FavoriteRelations struct {
	FavoriteDeviceIDs          map[string]bool
	FavoriteScenarioIDs        map[string]bool
	FavoriteGroupIDs           map[string]bool
	FavoriteDevicePropertyKeys DevicePropertyKeysMap
}

func (r FavoriteRelations) Merge(other FavoriteRelations) FavoriteRelations {
	result := NewEmptyFavoriteRelations()
	for deviceID := range other.FavoriteDeviceIDs {
		result.FavoriteDeviceIDs[deviceID] = true
	}
	for deviceID := range r.FavoriteDeviceIDs {
		result.FavoriteDeviceIDs[deviceID] = true
	}
	for scenarioID := range other.FavoriteScenarioIDs {
		result.FavoriteScenarioIDs[scenarioID] = true
	}
	for scenarioID := range r.FavoriteScenarioIDs {
		result.FavoriteScenarioIDs[scenarioID] = true
	}
	for groupID := range other.FavoriteGroupIDs {
		result.FavoriteGroupIDs[groupID] = true
	}
	for groupID := range r.FavoriteGroupIDs {
		result.FavoriteGroupIDs[groupID] = true
	}
	for devicePropertyKey := range other.FavoriteDevicePropertyKeys {
		result.FavoriteDevicePropertyKeys[devicePropertyKey] = true
	}
	for devicePropertyKey := range r.FavoriteDevicePropertyKeys {
		result.FavoriteDevicePropertyKeys[devicePropertyKey] = true
	}
	return result
}

type DevicePropertyKeysMap map[DevicePropertyKey]bool

func (f DevicePropertyKeysMap) MarshalJSON() ([]byte, error) {
	result := make(map[string]bool, len(f))
	for k, v := range f {
		result[fmt.Sprintf("%s:%s", k.DeviceID, k.PropertyKey)] = v
	}
	return json.Marshal(result)
}

func (f *DevicePropertyKeysMap) UnmarshalJSON(bytes []byte) error {
	rawMap := make(map[string]bool)
	if err := json.Unmarshal(bytes, &rawMap); err != nil {
		return err
	}
	result := make(DevicePropertyKeysMap, len(rawMap))
	for k, v := range rawMap {
		deviceIDAndPropertyKey := strings.SplitN(k, ":", 2)
		devicePropertyKey := DevicePropertyKey{
			DeviceID:    deviceIDAndPropertyKey[0],
			PropertyKey: deviceIDAndPropertyKey[1],
		}
		result[devicePropertyKey] = v
	}
	*f = result
	return nil
}

func (r FavoriteRelations) IsEmpty() bool {
	return len(r.FavoriteDeviceIDs) == 0 &&
		len(r.FavoriteScenarioIDs) == 0 &&
		len(r.FavoriteGroupIDs) == 0 &&
		len(r.FavoriteDevicePropertyKeys) == 0
}

func NewEmptyFavoriteRelations() FavoriteRelations {
	return FavoriteRelations{
		FavoriteDeviceIDs:          map[string]bool{},
		FavoriteScenarioIDs:        map[string]bool{},
		FavoriteGroupIDs:           map[string]bool{},
		FavoriteDevicePropertyKeys: map[DevicePropertyKey]bool{},
	}
}
