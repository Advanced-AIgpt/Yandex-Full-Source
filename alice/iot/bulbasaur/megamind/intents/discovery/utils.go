package discovery

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func containsProtocol(haystack []endpointpb.TIotDiscoveryCapability_TProtocol, needle endpointpb.TIotDiscoveryCapability_TProtocol) bool {
	for _, h := range haystack {
		if h == needle {
			return true
		}
	}
	return false
}

func containsDirective(haystack []endpointpb.TCapability_EDirectiveType, needle endpointpb.TCapability_EDirectiveType) bool {
	for _, h := range haystack {
		if h == needle {
			return true
		}
	}
	return false
}

func allDiscoveryDirectivesSupported(supportedDirectives []endpointpb.TCapability_EDirectiveType) bool {
	return containsDirective(supportedDirectives, endpointpb.TCapability_IotStartDiscoveryDirectiveType) &&
		containsDirective(supportedDirectives, endpointpb.TCapability_IotFinishDiscoveryDirectiveType)
}

func allUnlinkDirectivesSupported(supportedDirectives []endpointpb.TCapability_EDirectiveType) bool {
	return containsDirective(supportedDirectives, endpointpb.TCapability_IotForgetDevicesDirectiveType)
}

func getIntentState(ctx sdk.Context, dbClient db.DB, sessionID string) discovery.IntentState {
	user, ok := ctx.User()
	if !ok {
		ctx.Logger().Errorf("failed to get intent state: user not authorized")
		return discovery.IntentState{}
	}
	intentStateKey := model.NewIntentStateKey(ctx.ClientDeviceID(), sessionID, "discovery")
	rawState, err := dbClient.SelectUserIntentState(ctx.Context(), user.ID, intentStateKey)
	if err != nil {
		if xerrors.Is(err, &model.IntentStateNotFoundError{}) {
			ctx.Logger().Infof("no intent state is found, return empty intent state")
		} else {
			ctx.Logger().Errorf("failed to select user intent state: %w", err)
		}
		return discovery.IntentState{}
	}
	intentState := discovery.IntentState{}
	if err := json.Unmarshal(rawState, &intentState); err != nil {
		ctx.Logger().Errorf("failed to unmarshal user intent state: %w", err)
	}
	return intentState
}

func nlgForDiscoveryResult(discoveredDevices adapter.DeviceInfoViews, storeResults model.DeviceStoreResults) libnlg.NLG {
	switch {
	case len(storeResults) > 1:
		return finishDiscoveryMultipleResultNLG
	case len(storeResults) == 1:
		_, isButtonSensor := storeResults[0].Properties.GetByKey(model.PropertyKey(model.EventPropertyType, string(model.ButtonPropertyInstance)))
		_, isTemperatureSensor := storeResults[0].Properties.GetByKey(model.PropertyKey(model.FloatPropertyType, string(model.TemperaturePropertyInstance)))
		isSensor := storeResults[0].Type == model.SensorDeviceType
		switch {
		case isSensor && isButtonSensor:
			return finishDiscoveryButtonSensorNLG
		case isSensor && isTemperatureSensor:
			return finishDiscoveryTemperatureSensorNLG
		case isSensor:
			return finishDiscoverySensorNLG
		default:
			return finishDiscoverySingleResultNLG
		}
	case len(storeResults) == 0 && len(discoveredDevices) > 0:
		return finishDiscoveryUnsupportedDevicesNLG
	default:
		return finishDiscoveryNoResultsNLG
	}
}
