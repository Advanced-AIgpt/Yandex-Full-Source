package db

import (
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type rawFavorite struct {
	TargetID   string                 `json:"target_id"`
	Type       model.FavoriteType     `json:"type"`
	Key        string                 `json:"key"`
	Parameters iRawFavoriteParameters `json:"parameters"`
}

func (f *rawFavorite) FromScenario(s model.Scenario) {
	f.TargetID = s.ID
	f.Type = model.ScenarioFavoriteType
}

func (f *rawFavorite) FromDevice(d model.Device) {
	f.TargetID = d.ID
	f.Type = model.DeviceFavoriteType
}

func (f *rawFavorite) FromGroup(g model.Group) {
	f.TargetID = g.ID
	f.Type = model.GroupFavoriteType
}

func (f *rawFavorite) FromFavoritesDeviceProperty(property model.FavoritesDeviceProperty) {
	f.TargetID = property.DeviceID
	f.Type = model.DevicePropertyFavoriteType
	f.Key = property.PropertyKey()
	var parameters devicePropertyRawFavoriteParameters
	parameters.FromFavoritesDeviceProperty(property)
	f.Parameters = parameters
}

func (f *rawFavorite) FromProperty(deviceID string, property model.IProperty) {
	f.TargetID = deviceID
	f.Type = model.DevicePropertyFavoriteType
	f.Key = property.Key()
	var parameters devicePropertyRawFavoriteParameters
	parameters.FromProperty(property)
	f.Parameters = parameters
}

type iRawFavoriteParameters interface {
	isRawFavoriteParameters()
}

func JSONUnmarshalRawFavoriteParameters(fType model.FavoriteType, b []byte) (iRawFavoriteParameters, error) {
	switch fType {
	case model.DevicePropertyFavoriteType:
		params := devicePropertyRawFavoriteParameters{}
		if err := json.Unmarshal(b, &params); err != nil {
			return nil, err
		}
		return params, nil
	default:
		return nil, fmt.Errorf("unsupported favourite parameters type: %s", fType)
	}
}

type devicePropertyRawFavoriteParameters struct {
	Type     model.PropertyType     `json:"type"`
	Instance model.PropertyInstance `json:"instance"`
}

func (p devicePropertyRawFavoriteParameters) isRawFavoriteParameters() {}

func (p devicePropertyRawFavoriteParameters) PropertyKey() string {
	return model.PropertyKey(p.Type, string(p.Instance))
}

func (p *devicePropertyRawFavoriteParameters) FromProperty(property model.IProperty) {
	p.Type = property.Type()
	p.Instance = model.PropertyInstance(property.Instance())
}

func (p *devicePropertyRawFavoriteParameters) FromFavoritesDeviceProperty(favouriteProperty model.FavoritesDeviceProperty) {
	p.Type = favouriteProperty.Property.Type()
	p.Instance = model.PropertyInstance(favouriteProperty.Property.Instance())
}

type rawFavorites []rawFavorite

func (f rawFavorites) MakeFavorites(userInfo model.UserInfo) (model.Favorites, error) {
	devicesMap := userInfo.Devices.ToMap()
	scenariosMap := userInfo.Scenarios.ToMap()
	groupsMap := userInfo.Groups.ToMap()
	result := model.Favorites{
		Scenarios:  make(model.Scenarios, 0),
		Devices:    make(model.Devices, 0),
		Groups:     make(model.Groups, 0),
		Properties: make([]model.FavoritesDeviceProperty, 0),
	}
	for _, favorite := range f {
		switch favorite.Type {
		case model.DeviceFavoriteType:
			if device, exist := devicesMap[favorite.TargetID]; exist {
				result.Devices = append(result.Devices, device.Clone())
			}
		case model.ScenarioFavoriteType:
			if scenario, exist := scenariosMap[favorite.TargetID]; exist {
				result.Scenarios = append(result.Scenarios, scenario.Clone())
			}
		case model.GroupFavoriteType:
			if group, exist := groupsMap[favorite.TargetID]; exist {
				result.Groups = append(result.Groups, group.Clone())
			}
		case model.DevicePropertyFavoriteType:
			device, exist := devicesMap[favorite.TargetID]
			if !exist {
				continue
			}
			devicePropertiesMap := device.Properties.AsMap()
			favoriteDevicePropertyParameters, ok := favorite.Parameters.(devicePropertyRawFavoriteParameters)
			if !ok {
				return model.Favorites{}, xerrors.New("failed to cast favorite device property parameters")
			}
			if deviceProperty, exist := devicePropertiesMap[favoriteDevicePropertyParameters.PropertyKey()]; exist {
				var favoriteProperty model.FavoritesDeviceProperty
				favoriteProperty.FromProperty(device.ID, deviceProperty)
				result.Properties = append(result.Properties, favoriteProperty)
			}
		default:
			return model.Favorites{}, xerrors.Errorf("unknown favorites type: %s", favorite.Type)
		}
	}
	return result, nil
}

func (f rawFavorites) toFavoriteRelationMaps() model.FavoriteRelations {
	favoriteDeviceIDs := make(map[string]bool)
	favoriteGroupIDs := make(map[string]bool)
	favoriteScenarioIDs := make(map[string]bool)
	favoriteDevicePropertyKeys := make(map[model.DevicePropertyKey]bool)

	for _, favorite := range f {
		switch favorite.Type {
		case model.DeviceFavoriteType:
			favoriteDeviceIDs[favorite.TargetID] = true
		case model.ScenarioFavoriteType:
			favoriteScenarioIDs[favorite.TargetID] = true
		case model.GroupFavoriteType:
			favoriteGroupIDs[favorite.TargetID] = true
		case model.DevicePropertyFavoriteType:
			favoriteDevicePropertyKeys[model.DevicePropertyKey{DeviceID: favorite.TargetID, PropertyKey: favorite.Key}] = true
		}
	}
	return model.FavoriteRelations{
		FavoriteDeviceIDs:          favoriteDeviceIDs,
		FavoriteScenarioIDs:        favoriteScenarioIDs,
		FavoriteGroupIDs:           favoriteGroupIDs,
		FavoriteDevicePropertyKeys: favoriteDevicePropertyKeys,
	}
}
