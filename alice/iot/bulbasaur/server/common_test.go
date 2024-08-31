package server

import (
	"fmt"
	"net/http"
	"strconv"
	"testing"
	"time"

	"github.com/google/go-cmp/cmp"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/xiva"
)

func TestDeviceCopy(t *testing.T) {
	deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	deviceOnOff.SetRetrievable(true)
	deviceOnOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})
	device := model.Device{
		ID:         "1",
		ExternalID: "ext1",
		Name:       "Name",
		Type:       model.LightDeviceType,
		SkillID:    "111",
		Capabilities: []model.ICapability{
			deviceOnOff,
		},
		Room: &model.Room{
			ID:   "room1",
			Name: "Room",
		},
	}

	newDeviceState := adapter.DeviceStateView{
		ID: "ext1",
		Capabilities: []adapter.CapabilityStateView{{
			Type: model.OnOffCapabilityType,
			State: model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			},
		}},
	}

	//deepcopy of struct
	updatedDevice2 := device.Clone()
	updatedDevice2.UpdateState(newDeviceState.ToDevice().Capabilities.WithLastUpdated(1), nil)
	assert.False(t, cmp.Equal(device, updatedDevice2))

	//copies struct, but keeps pointers to slices/maps
	updatedDevice := device
	updatedDevice.UpdateState(newDeviceState.ToDevice().Capabilities, nil)
	assert.True(t, cmp.Equal(device, updatedDevice))

	//clones should be equal
	clone := device.Clone()
	assert.True(t, cmp.Equal(device, clone))
}

func (suite *ServerSuite) TestQuery() {
	suite.Run("OverrideCapabilityStateFromProvider", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
										},
									},
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": true
				},
				"parameters": {"split": false}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DeviceUnreachable", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		queryTimestamp1 := timestamp.PastTimestamp(1917)
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})
		lampVoltage := model.MakePropertyByType(model.FloatPropertyType)
		lampVoltage.SetParameters(model.FloatPropertyParameters{
			Instance: model.VoltagePropertyInstance,
			Unit:     model.UnitVolt,
		})
		lampVoltage.SetRetrievable(true)
		lampVoltage.SetState(model.FloatPropertyState{
			Instance: model.VoltagePropertyInstance,
			Value:    220,
		})
		lampVoltage.SetStateChangedAt(queryTimestamp1)
		lampVoltage.SetLastUpdated(queryTimestamp1)
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(lampOnOff).
				WithProperties(lampVoltage),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:           lamp.ExternalID,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: string(adapter.DeviceUnreachable),
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "offline",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
            "properties": [{
				"type": "devices.properties.float",
				"reportable": false,
				"retrievable": true,
				"state": null,
				"parameters": {
					"instance": "voltage",
					"name": "текущее напряжение",
					"unit": "unit.volt"
				},
				"state_changed_at": "1970-01-01T00:31:57Z",
				"last_updated": "1970-01-01T00:31:57Z"
			}],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DeviceBusy", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:           lamp.ExternalID,
									ErrorCode:    adapter.DeviceBusy,
									ErrorMessage: string(adapter.DeviceBusy),
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DeviceNotFound", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:           lamp.ExternalID,
									ErrorCode:    adapter.DeviceNotFound,
									ErrorMessage: string(model.DeviceNotFound),
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "not_found",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DeviceInvalidState", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		lampRange.SetRetrievable(true)
		lampRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: false,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			})
		lampRange.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    25,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampRange,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.RangeCapabilityType,
											State: &model.RangeCapabilityState{
												Instance: model.BrightnessRangeInstance,
												Relative: tools.AOB(false),
												Value:    -1,
											},
										},
									},
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.range",
				"retrievable": true,
				"state": {
					"instance": "brightness",
					"value": 25
				},
				"parameters": {
					"instance": "brightness",
					"name": "яркость",
					"unit": "unit.percent",
					"random_access": false,
					"looped": false,
					"range": {
						"min": 1,
						"max": 100,
						"precision": 1
					}
				}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DevicePartialStateUpdate", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		lampRange.SetRetrievable(true)
		lampRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: false,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			})
		lampRange.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    25,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampRange,
				),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
										},
										{
											Type: model.RangeCapabilityType,
											State: &model.RangeCapabilityState{
												Instance: model.BrightnessRangeInstance,
												Value:    10,
											},
										},
									},
									Properties: []adapter.PropertyStateView{
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.VoltagePropertyInstance,
												Value:    220,
											},
										},
									},
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.range",
				"retrievable": true,
				"state": {
					"instance": "brightness",
					"value": 10
				},
				"parameters": {
					"instance": "brightness",
					"name": "яркость",
					"unit": "unit.percent",
					"random_access": false,
					"looped": false,
					"range": {
						"min": 1,
						"max": 100,
						"precision": 1
					}
				}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("DeviceOnlyPropertiesUpgrade", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		waterLevel := model.MakePropertyByType(model.FloatPropertyType)
		waterLevel.SetRetrievable(true)
		waterLevel.SetParameters(model.FloatPropertyParameters{
			Instance: model.WaterLevelPropertyInstance,
			Unit:     model.UnitPercent,
		})
		waterLevel.SetState(model.FloatPropertyState{
			Instance: model.WaterLevelPropertyInstance,
			Value:    66,
		})
		bathtub, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Ванная").
				WithSkillID("testProvider").
				WithProperties(waterLevel),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: bathtub.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
										},
										{
											Type: model.RangeCapabilityType,
											State: &model.RangeCapabilityState{
												Instance: model.BrightnessRangeInstance,
												Value:    10,
											},
										},
									},
									Properties: []adapter.PropertyStateView{
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.WaterLevelPropertyInstance,
												Value:    33,
											},
										},
									},
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", bathtub.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Ванная",
			"names": ["Ванная"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": [],
			"capabilities": [],
            "properties": [{
				"type": "devices.properties.float",
				"reportable": false,
				"retrievable": true,
				"parameters": {
					"instance": "water_level",
					"name": "уровень воды",
					"unit": "unit.percent"
				},
				"state": {
					"percent": 33,
					"status": "warning",
					"value": 33
				},
				"state_changed_at": "1970-01-01T00:00:01Z",
				"last_updated": "1970-01-01T00:00:01Z"
			}
			],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, bathtub.ID, bathtub.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

	suite.Run("ColorSettingTest", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		lampColor.SetRetrievable(true)
		lampColor.SetParameters(
			model.ColorSettingCapabilityParameters{
				TemperatureK: &model.TemperatureKParameters{
					Min: 2700,
					Max: 6500,
				},
				ColorSceneParameters: &model.ColorSceneParameters{
					Scenes: model.ColorScenes{
						{
							ID: model.ColorSceneIDSiren,
						},
						{
							ID: model.ColorSceneIDParty,
						},
					},
				},
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithCapabilities(
					lampOnOff,
					lampColor,
				),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.ColorSettingCapabilityType,
											State: &model.ColorSettingCapabilityState{
												Instance: model.TemperatureKCapabilityInstance,
												Value:    model.TemperatureK(3357), // closest value is 3400 - warm_white
											},
											Timestamp: server.timestamper.CurrentTimestamp(),
										},
									},
								},
							},
						},
					},
					"query-2": {
						RequestID: "query-2",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.ColorSettingCapabilityType,
											State: &model.ColorSettingCapabilityState{
												Instance: model.TemperatureKCapabilityInstance,
												Value:    model.TemperatureK(3302), // closest value is 3400 - ColorIDWarmWhite
											},
											Timestamp: server.timestamper.CurrentTimestamp() + 1,
										},
									},
								},
							},
						},
					},
					"query-3": {
						RequestID: "query-3",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.ColorSettingCapabilityType,
											State: &model.ColorSettingCapabilityState{
												Instance: model.SceneCapabilityInstance,
												Value:    model.ColorSceneIDSiren,
											},
											Timestamp: server.timestamper.CurrentTimestamp() + 2,
										},
									},
								},
							},
						},
					},
				},
			)

		suite.Run("query-1", func() {
			request := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
				withRequestID("query-1").
				withBlackboxUser(&alice.User)

			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "query-1",
				"id": "%s",
				"name": "Лампочка",
				"names": ["Лампочка"],
				"type": "",
				"icon_url": "",
				"state": "online",
				"groups": [],
				"capabilities": [
					{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"state": {
							"instance": "on",
							"value": false
						},
						"parameters": {"split": false}
					},
					{
						"retrievable": true,
						"type": "devices.capabilities.color_setting",
						"state": {
							"instance": "color",
							"value": {
								"id": "warm_white",
								"name": "Теплый белый",
								"type": "white",
								"value": {
									"h": 33,
									"s": 49,
									"v": 100
								}
							}
						},
						"parameters": {
							"instance": "color",
							"name": "цвет",
							"scenes": [
								{
									"id": "party",
									"name": "Вечеринка"
								},
								{
									"id": "siren",
									"name": "Сирена"
								}
							],
							"palette": [
								{
									"id": "soft_white",
									"name": "Мягкий белый",
									"type": "white",
									"value": {
										"h": 32,
										"s": 67,
										"v": 100
									}
								},
								{
									"id": "warm_white",
									"name": "Теплый белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 49,
										"v": 100
									}
								},
								{
									"id": "white",
									"name": "Белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 28,
										"v": 100
									}
								},
								{
									"id": "daylight",
									"name": "Дневной белый",
									"type": "white",
									"value": {
										"h": 27,
										"s": 11,
										"v": 100
									}
								},
								{
									"id": "cold_white",
									"name": "Холодный белый",
									"type": "white",
									"value": {
										"h": 340,
										"s": 2,
										"v": 100
									}
								}
							]
						}
					}
				],
                "properties": [],
				"skill_id": "testProvider",
				"external_id": "%s",
				"favorite": false
			}
			`, lamp.ID, lamp.ExternalID)

			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		})
		suite.Run("query-2", func() {
			request := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
				withRequestID("query-2").
				withBlackboxUser(&alice.User)

			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "query-2",
				"id": "%s",
				"name": "Лампочка",
				"names": ["Лампочка"],
				"type": "",
				"icon_url": "",
				"state": "online",
				"groups": [],
				"capabilities": [
					{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"state": {
							"instance": "on",
							"value": false
						},
						"parameters": {"split": false}
					},
					{
						"retrievable": true,
						"type": "devices.capabilities.color_setting",
						"state": {
							"instance": "color",
							"value": {
								"id": "warm_white",
								"name": "Теплый белый",
								"type": "white",
								"value": {
									"h": 33,
									"s": 49,
									"v": 100
								}
							}
						},
						"parameters": {
							"instance": "color",
							"name": "цвет",
							"scenes": [
								{
									"id": "party",
									"name": "Вечеринка"
								},
								{
									"id": "siren",
									"name": "Сирена"
								}
							],
							"palette": [
								{
									"id": "soft_white",
									"name": "Мягкий белый",
									"type": "white",
									"value": {
										"h": 32,
										"s": 67,
										"v": 100
									}
								},
								{
									"id": "warm_white",
									"name": "Теплый белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 49,
										"v": 100
									}
								},
								{
									"id": "white",
									"name": "Белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 28,
										"v": 100
									}
								},
								{
									"id": "daylight",
									"name": "Дневной белый",
									"type": "white",
									"value": {
										"h": 27,
										"s": 11,
										"v": 100
									}
								},
								{
									"id": "cold_white",
									"name": "Холодный белый",
									"type": "white",
									"value": {
										"h": 340,
										"s": 2,
										"v": 100
									}
								}
							]
						}
					}
				],
            	"properties": [],
				"skill_id": "testProvider",
				"external_id": "%s",
				"favorite": false
			}
			`, lamp.ID, lamp.ExternalID)

			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		})
		suite.Run("query-3", func() {
			request := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
				withRequestID("query-3").
				withBlackboxUser(&alice.User)

			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "query-3",
				"id": "%s",
				"name": "Лампочка",
				"names": ["Лампочка"],
				"type": "",
				"icon_url": "",
				"state": "online",
				"groups": [],
				"capabilities": [
					{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"state": {
							"instance": "on",
							"value": false
						},
						"parameters": {"split": false}
					},
					{
						"retrievable": true,
						"type": "devices.capabilities.color_setting",
						"state": {
							"instance": "scene",
							"value": {
								"id": "siren",
								"name": "Сирена"
							}
						},
						"parameters": {
							"instance": "color",
							"name": "цвет",
							"scenes": [
								{
									"id": "party",
									"name": "Вечеринка"
								},
								{
									"id": "siren",
									"name": "Сирена"
								}
							],
							"palette": [
								{
									"id": "soft_white",
									"name": "Мягкий белый",
									"type": "white",
									"value": {
										"h": 32,
										"s": 67,
										"v": 100
									}
								},
								{
									"id": "warm_white",
									"name": "Теплый белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 49,
										"v": 100
									}
								},
								{
									"id": "white",
									"name": "Белый",
									"type": "white",
									"value": {
										"h": 33,
										"s": 28,
										"v": 100
									}
								},
								{
									"id": "daylight",
									"name": "Дневной белый",
									"type": "white",
									"value": {
										"h": 27,
										"s": 11,
										"v": 100
									}
								},
								{
									"id": "cold_white",
									"name": "Холодный белый",
									"type": "white",
									"value": {
										"h": 340,
										"s": 2,
										"v": 100
									}
								}
							]
						}
					}
				],
            	"properties": [],
				"skill_id": "testProvider",
				"external_id": "%s",
				"favorite": false
			}
			`, lamp.ID, lamp.ExternalID)

			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		})
	})

	suite.Run("TokenNotFound", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithClientError(&socialism.TokenNotFoundError{})

		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "not_found",
			"groups": [],
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)
		suite.JSONResponseMatch(server, requestData, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestDiscovery() {
	suite.RunServerTest("RichDiscovery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		defer func(limit uint64) {
			model.ConstDeviceLimit = limit
		}(model.ConstDeviceLimit)
		model.ConstDeviceLimit = 2

		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		lamp1, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("Лампа 1").WithSkillID("xiaomi"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"rich-discovery": {
						RequestID: "rich-discovery",
						//Timestamp: 0,
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:           lamp1.ExternalID,
									Name:         "Лампа 1",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         model.LightDeviceType,
								},
								{
									ID:           "rich-discovery-lamp-2",
									Name:         "Лампа 2",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         model.LightDeviceType,
								},
								{
									ID:           "rich-discovery-lamp-3",
									Name:         "Лампа 3",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         model.LightDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("rich-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "rich-discovery-lamp-2", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "rich-discovery",
			"new_device_count": 1,
			"updated_device_count": 1,
			"limit_device_count": 1,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})

	suite.RunServerTest("SideEffects", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		xiaomiProvider := server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"discovery-1": {
						RequestID: "discovery-1",
						Payload: adapter.DiscoveryPayload{
							UserID: "external-alice-id-1",
							Devices: []adapter.DeviceInfoView{
								{
									ID:           "lamp-external-id-1",
									Name:         "Lamp 1",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         model.LightDeviceType,
								},
							},
						},
					},
				},
			)

		// check we had no external users before
		suite.CheckExternalUsers(server, "external-alice-id-1", "xiaomi", []*model.User{})
		// check we have no devices before
		suite.CheckUserDevices(server, &alice.User, []model.Device{})

		request := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("discovery-1").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "lamp-external-id-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "discovery-1",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		// check number of requests to provider
		expectedDiscoveryCalls := 1
		suite.Equal(expectedDiscoveryCalls, xiaomiProvider.DiscoveryCalls("discovery-1"), server.Logs())

		// check external user is stored
		expectedExternalUsers := []*model.User{&alice.User}
		suite.CheckExternalUsers(server, "external-alice-id-1", "xiaomi", expectedExternalUsers)

		// check discovered devices are stored
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		expectedDevices := []model.Device{
			{
				Name:         "Lamp 1",
				Aliases:      []string{},
				ExternalID:   "lamp-external-id-1",
				ExternalName: "Lamp 1",
				SkillID:      "xiaomi",
				Type:         "devices.types.light",
				OriginalType: "devices.types.light",
				Capabilities: []model.ICapability{
					lampOnOff,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo:  &model.DeviceInfo{},
				Updated:     server.dbClient.CurrentTimestamp(),
				Created:     server.dbClient.CurrentTimestamp(),
				Status:      model.UnknownDeviceStatus,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)

		suite.NoError(server.xiva.AssertEvent(100*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
			suite.Equal(alice.ID, actualEvent.UserID)
			suite.EqualValues(updates.UpdateDeviceListEventID, actualEvent.EventID)

			event := actualEvent.EventData.Payload.(updates.UpdateDeviceListEvent)
			suite.Equal(updates.DiscoverySource, event.Source)
			return nil
		}))
	})

	suite.RunServerTest("ValidateDiscoveryResult", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"partially-valid-discovery": {
						RequestID: "partially-valid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:           "invalid-device-type-1",
									Name:         "Лампа 1",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         "unknown.device.type",
								},
								{
									ID:           "lamp-1",
									Name:         "Лампа 2",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         model.LightDeviceType,
								},
								{
									ID:           "smart-ukulele",
									Name:         "Ukulele",
									Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
									Type:         "smart.ukulele.device.type",
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("partially-valid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "lamp-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "partially-valid-discovery",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})

	suite.RunServerTest("ValidateRemoteCarToggles", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		remoteCarToggle := adapter.CapabilityInfoView{
			Retrievable: true,
			Type:        model.ToggleCapabilityType,
			Parameters:  model.ToggleCapabilityParameters{Instance: model.TrunkToggleCapabilityInstance},
		}

		server.pfMock.NewProvider(&alice.User, model.REMOTECAR, true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"valid-discovery": {
						RequestID: "valid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "lamp-1",
									Name: "Лампа 2",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
										remoteCarToggle,
									},
									Type: model.LightDeviceType,
								},
							},
						},
					},
				},
			)

		// non-remotecar provider
		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"invalid-discovery": {
						RequestID: "invalid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "lamp-1",
									Name: "Лампа 2",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
										remoteCarToggle,
									},
									Type: model.LightDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/RC/discovery").
			withRequestID("valid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "lamp-1", "RC")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "valid-discovery",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		request = newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("invalid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody = server.doRequest(request)
		expectedBody = `{
			"status": "ok",
			"request_id": "invalid-discovery",
			"new_device_count": 0,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": []
		}`
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})

	suite.RunServerTest("ValidateRemoteCarDeviceTypes", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, model.REMOTECAR, true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"valid-discovery": {
						RequestID: "valid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "car-1",
									Name: "Машинка",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
									},
									Type: model.RemoteCarDeviceType,
								},
							},
						},
					},
				},
			)

		// non-remotecar provider
		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"invalid-discovery": {
						RequestID: "invalid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "car-1",
									Name: "Китайская машина",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
									},
									Type: model.RemoteCarDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/RC/discovery").
			withRequestID("valid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "car-1", "RC")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "valid-discovery",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		request = newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("invalid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody = server.doRequest(request)
		expectedBody = `{
			"status": "ok",
			"request_id": "invalid-discovery",
			"new_device_count": 0,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": []
		}`
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})

	suite.RunServerTest("ValidateQuasarDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, model.QUASAR, true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"valid-discovery": {
						RequestID: "valid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "speaker-1",
									Name: "Колонка",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
									},
									Type: model.YandexStationDeviceType,
								},
							},
						},
					},
				},
			)

		// non-quasar provider
		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"invalid-discovery": {
						RequestID: "invalid-discovery",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "speaker-1",
									Name: "Колонка",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
									},
									Type: model.YandexStationDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/Q/discovery").
			withRequestID("valid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "speaker-1", "Q")
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "valid-discovery",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		request = newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("invalid-discovery").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody = server.doRequest(request)
		expectedBody = `{
			"status": "ok",
			"request_id": "invalid-discovery",
			"new_device_count": 0,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": []
		}`
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})
}

func (suite *ServerSuite) TestCurtainIntegration() {
	suite.RunServerTest("CurtainDeviceIntegration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		server.timestamperFactory.WithCreatedTimestamp(2)
		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"curtain-discovery-1": {
						RequestID: "curtain-discovery-1",
						Timestamp: timestamp.CurrentTimestampMock,
						Payload: adapter.DiscoveryPayload{
							UserID: strconv.FormatUint(alice.ID, 10),
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "curtain-external-id-1",
									Name: "Штора",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.OnOffCapability(),
									},
									Type: model.CurtainDeviceType,
								},
							},
						},
					},
				},
			).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"curtain-query-1": {
						RequestID: "curtain-query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: "curtain-external-id-1",
									Capabilities: []adapter.CapabilityStateView{
										xtestadapter.OnOffState(false, timestamp.CurrentTimestampMock),
									},
								},
							},
						},
					},
				},
			).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"curtain-action-1": {
						RequestID: "curtain-action-1",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: "curtain-external-id-1",
									Capabilities: []adapter.CapabilityActionResultView{
										xtestadapter.OnOffActionSuccessResult(2),
									},
								},
							},
						},
					},
				})

		discoveryRequest := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("curtain-discovery-1").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(discoveryRequest)
		curtainOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		curtainOnOff.SetRetrievable(true)
		curtainOnOff.SetParameters(model.OnOffCapabilityParameters{Split: false})
		expectedDevices := []model.Device{
			{
				Name:         "Штора",
				Aliases:      []string{},
				ExternalID:   "curtain-external-id-1",
				ExternalName: "Штора",
				SkillID:      "xiaomi",
				Type:         model.CurtainDeviceType,
				OriginalType: model.CurtainDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: []model.ICapability{
					curtainOnOff,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{},
				Updated:    timestamp.CurrentTimestampMock,
				Created:    timestamp.CurrentTimestampMock,
				Status:     model.UnknownDeviceStatus,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "curtain-external-id-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedDiscoveryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "curtain-discovery-1",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id": "%s",
					"name": "%s",
					"type": "%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedDiscoveryResponseBody, actualBody)

		server.inflector.WithInflection("Штора", inflector.Inflection{Vin: "Шторы", Tvor: "Шторами"})
		suggestionsRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID, "suggestions")).
			withRequestID("curtain-suggestions-1").
			withBlackboxUser(&alice.User)
		expectedSuggestionsResponseBody := `{
			"status": "ok",
			"request_id": "curtain-suggestions-1",
			"commands": [
				{
					"type": "toggles",
					"name": "Включение и выключение",
					"suggests": ["Открой шторы","Закрой шторы"]
				},
				{
					"type": "queries",
					"name": "Проверка статуса",
					"suggests": ["Что с шторами?"]
				},
				{
					"type": "timers",
					"name": "Отложенные команды",
					"suggests": [
						"Открой шторы на 15 минут",
						"Закрой шторы через 2 часа",
						"Отмени все команды"
					]
				}
			]
		}`
		suite.JSONResponseMatch(server, suggestionsRequest, http.StatusOK, expectedSuggestionsResponseBody)

		queryRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("curtain-query-1").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "curtain-query-1",
			"id": "%s",
			"name": "Штора",
			"names": ["Штора"],
			"type": "devices.types.openable.curtain",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
			"properties": [],
			"skill_id": "xiaomi",
			"external_id": "curtain-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedQueryResponseBody)

		actionRequest := newRequest("POST", tools.URLJoin("/m/user/devices/", externalDevice.ID, "actions")).
			withRequestID("curtain-action-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.OnOffCapabilityType,
						"state": JSONObject{
							"instance": model.OnOnOffCapabilityInstance,
							"value":    true,
						},
					},
				},
			})
		expectedActionResponseBody := fmt.Sprintf(`{
			"request_id":"curtain-action-1",
			"status":"ok",
	       "devices":[{
	           "id":"%s",
			    "capabilities":[{
	   			"type":"devices.capabilities.on_off",
	               "state":{
	   				"instance":"on",
		        		"action_result":{"status":"DONE"}
				    }
		        }]
	       }]
		}`, externalDevice.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)

		curtainOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})
		curtainOnOff.SetLastUpdated(timestamp.CurrentTimestampMock + 1)
		expectedDevices = []model.Device{
			{
				Name:         "Штора",
				Aliases:      []string{},
				ExternalID:   "curtain-external-id-1",
				ExternalName: "Штора",
				SkillID:      "xiaomi",
				Type:         model.CurtainDeviceType,
				OriginalType: model.CurtainDeviceType,
				Capabilities: []model.ICapability{
					curtainOnOff,
				},
				HouseholdID:   currentHousehold.ID,
				Properties:    model.Properties{},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       timestamp.CurrentTimestampMock,
				Created:       timestamp.CurrentTimestampMock,
				Status:        model.OnlineDeviceStatus,
				StatusUpdated: timestamp.CurrentTimestampMock,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
	})
}

func (suite *ServerSuite) TestDeviceIntegration() {
	suite.RunServerTest("ToggleDeviceIntegration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		server.timestamperFactory.WithCreatedTimestamp(2)
		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		virtualTogglerBacklight := model.MakeCapabilityByType(model.ToggleCapabilityType)
		virtualTogglerBacklight.SetRetrievable(true)
		virtualTogglerBacklight.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		virtualTogglerIonization := model.MakeCapabilityByType(model.ToggleCapabilityType)
		virtualTogglerIonization.SetRetrievable(true)
		virtualTogglerIonization.SetParameters(model.ToggleCapabilityParameters{Instance: model.IonizationToggleCapabilityInstance})
		virtualToggler, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("Джин").
			WithCapabilities(
				virtualTogglerBacklight,
				virtualTogglerIonization,
			).UpdatedAt(
			timestamp.CurrentTimestampMock,
		))

		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"toggle-discovery-1": {
						RequestID: "toggle-discovery-1",
						Timestamp: timestamp.CurrentTimestampMock,
						Payload: adapter.DiscoveryPayload{
							UserID: strconv.FormatUint(alice.ID, 10),
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "toggle-external-id-1",
									Name: "Тогглер",
									Capabilities: []adapter.CapabilityInfoView{
										xtestadapter.ToggleCapability(model.ControlsLockedToggleCapabilityInstance),
									},
									Type: model.OtherDeviceType,
								},
							},
						},
					},
				},
			).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"toggle-query-1": {
						RequestID: "toggle-query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: "toggle-external-id-1",
									Capabilities: []adapter.CapabilityStateView{
										xtestadapter.ToggleState(model.ControlsLockedToggleCapabilityInstance, false, 1),
									},
								},
							},
						},
					},
				},
			).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"toggle-action-1": {
						RequestID: "toggle-action-1",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: "toggle-external-id-1",
									Capabilities: []adapter.CapabilityActionResultView{
										xtestadapter.ToggleActionSuccessResult(model.ControlsLockedToggleCapabilityInstance, 3),
									},
								},
							},
						},
					},
				})

		discoveryRequest := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("toggle-discovery-1").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(discoveryRequest)

		xiaomiTogglerControls := model.MakeCapabilityByType(model.ToggleCapabilityType)
		xiaomiTogglerControls.SetRetrievable(true)
		xiaomiTogglerControls.SetParameters(model.ToggleCapabilityParameters{Instance: model.ControlsLockedToggleCapabilityInstance})
		expectedDevices := []model.Device{
			{
				Name:         "Тогглер",
				Aliases:      []string{},
				ExternalID:   "toggle-external-id-1",
				ExternalName: "Тогглер",
				SkillID:      "xiaomi",
				Type:         model.OtherDeviceType,
				OriginalType: model.OtherDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: []model.ICapability{
					xiaomiTogglerControls,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{},
				Updated:    timestamp.CurrentTimestampMock,
				Created:    timestamp.CurrentTimestampMock,
				Status:     model.UnknownDeviceStatus,
			},
			{
				Name:         "Джин",
				Aliases:      []string{},
				ExternalID:   virtualToggler.ExternalID,
				ExternalName: "Джин",
				SkillID:      "VIRTUAL",
				OriginalType: model.OtherDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: []model.ICapability{
					virtualTogglerBacklight,
					virtualTogglerIonization,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{},
				Updated:    timestamp.CurrentTimestampMock,
				Created:    timestamp.CurrentTimestampMock,
				Status:     model.UnknownDeviceStatus,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "toggle-external-id-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedDiscoveryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "toggle-discovery-1",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id": "%s",
					"name": "%s",
					"type": "%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedDiscoveryResponseBody, actualBody)

		server.inflector.WithInflection("Тогглер", inflector.Inflection{Rod: "Тогглера", Tvor: "Тогглером", Pr: "Тогглере"})
		suggestionsRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID, "suggestions")).
			withBlackboxUser(&alice.User)
		expectedSuggestionsResponseBody := `{
			"status": "ok",
			"request_id": "default-req-id",
			"commands": [
				{
					"type": "toggles",
					"name": "Включение и выключение",
					"suggests": [
						"Выключи детский режим на тогглере",
						"Выключи детский режим тогглера",
						"Включи детский режим на тогглере",
						"Включи детский режим тогглера",
						"Заблокируй управление тогглера",
						"Разблокируй управление тогглера",
						"Разблокируй управление на тогглере",
						"Заблокируй управление на тогглере"
					]
				},
				{
					"type": "queries",
					"name": "Проверка статуса",
					"suggests": ["Что с тогглером?", "Блокировка управления на тогглере включена?"]
				}
			]
		}`
		suite.JSONResponseMatch(server, suggestionsRequest, http.StatusOK, expectedSuggestionsResponseBody)

		queryRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("toggle-query-1").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "toggle-query-1",
			"id": "%s",
			"name": "Тогглер",
			"names": ["Тогглер"],
			"type": "devices.types.other",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [{
				"type": "devices.capabilities.toggle",
				"retrievable": true,
				"state": {
					"instance": "controls_locked",
					"value": false
				},
				"parameters": {
					"instance": "controls_locked",
					"name": "блокировка управления"
				}
			}],
			"properties": [],
			"skill_id": "xiaomi",
			"external_id": "toggle-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedQueryResponseBody)

		server.timestamperFactory.WithCreatedTimestamp(3)
		actionRequest := newRequest("POST", tools.URLJoin("/m/user/devices/", externalDevice.ID, "actions")).
			withRequestID("toggle-action-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.ToggleCapabilityType,
						"state": JSONObject{
							"instance": model.ControlsLockedToggleCapabilityInstance,
							"value":    true,
						},
					},
				},
			})
		expectedActionResponseBody := fmt.Sprintf(`{
			"request_id":"toggle-action-1",
			"status":"ok",
            "devices":[{
                "id":"%s",
			    "capabilities":[{
        			"type":"devices.capabilities.toggle",
                    "state":{
        				"instance":"controls_locked",
		        		"action_result":{"status":"DONE"}
				    }
		        }]
            }]
		}`, externalDevice.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)

		xiaomiTogglerControls.SetState(
			model.ToggleCapabilityState{
				Instance: model.ControlsLockedToggleCapabilityInstance,
				Value:    true,
			})
		xiaomiTogglerControls.SetLastUpdated(3)
		expectedDevices = []model.Device{
			{
				Name:         "Тогглер",
				Aliases:      []string{},
				ExternalID:   "toggle-external-id-1",
				ExternalName: "Тогглер",
				SkillID:      "xiaomi",
				Type:         model.OtherDeviceType,
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					xiaomiTogglerControls,
				},
				HouseholdID:   currentHousehold.ID,
				Properties:    model.Properties{},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       timestamp.CurrentTimestampMock,
				Created:       timestamp.CurrentTimestampMock,
				Status:        model.OnlineDeviceStatus,
				StatusUpdated: timestamp.CurrentTimestampMock,
			},
			{
				Name:         "Джин",
				Aliases:      []string{},
				ExternalID:   virtualToggler.ExternalID,
				ExternalName: "Джин",
				SkillID:      "VIRTUAL",
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					virtualTogglerBacklight,
					virtualTogglerIonization,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo:  &model.DeviceInfo{},
				Updated:     timestamp.CurrentTimestampMock,
				Created:     timestamp.CurrentTimestampMock,
				Status:      model.UnknownDeviceStatus,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)

		actionRequest = newRequest("POST", tools.URLJoin("/m/user/devices/", virtualToggler.ID, "actions")).
			withRequestID("toggle-action-2").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.ToggleCapabilityType,
						"state": JSONObject{
							"instance": model.IonizationToggleCapabilityInstance,
							"value":    true,
						},
					},
				},
			})
		expectedActionResponseBody = fmt.Sprintf(`{
			"request_id":"toggle-action-2",
			"status":"ok",
            "devices":[{
                "id":"%s",
			    "capabilities":[{
        			"type":"devices.capabilities.toggle",
                    "state":{
        				"instance":"ionization",
		        		"action_result":{"status":"DONE"}
				    }
		        }]
            }]
		}`, virtualToggler.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)

		virtualTogglerIonization.SetState(
			model.ToggleCapabilityState{
				Instance: model.IonizationToggleCapabilityInstance,
				Value:    true,
			})
		virtualTogglerIonization.SetLastUpdated(3)
		expectedDevices = []model.Device{
			{
				Name:         "Тогглер",
				Aliases:      []string{},
				ExternalID:   "toggle-external-id-1",
				ExternalName: "Тогглер",
				SkillID:      "xiaomi",
				Type:         model.OtherDeviceType,
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					xiaomiTogglerControls,
				},
				HouseholdID:   currentHousehold.ID,
				Properties:    model.Properties{},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       timestamp.CurrentTimestampMock,
				Created:       timestamp.CurrentTimestampMock,
				Status:        model.OnlineDeviceStatus,
				StatusUpdated: timestamp.CurrentTimestampMock,
			},
			{
				Name:         "Джин",
				Aliases:      []string{},
				ExternalID:   virtualToggler.ExternalID,
				ExternalName: "Джин",
				SkillID:      "VIRTUAL",
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					virtualTogglerBacklight,
					virtualTogglerIonization,
				},
				HouseholdID:   currentHousehold.ID,
				Properties:    model.Properties{},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       timestamp.CurrentTimestampMock,
				Created:       timestamp.CurrentTimestampMock,
				Status:        model.OnlineDeviceStatus, // status is updated for virtual providers too
				StatusUpdated: timestamp.CurrentTimestampMock,
			},
		}
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
	})

	suite.RunServerTest("HumidifierDeviceIntegration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		var (
			discoveryTimestamp timestamp.PastTimestamp = 1905
			queryTimestamp1    timestamp.PastTimestamp = 1917
			actionTimestamp    timestamp.PastTimestamp = 1991
			queryTimestamp2    timestamp.PastTimestamp = 2000
		)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		humidityRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		humidityRange.SetRetrievable(true)
		humidityRange.SetParameters(model.RangeCapabilityParameters{
			Instance:     model.HumidityRangeInstance,
			Unit:         model.UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &model.Range{
				Min:       10,
				Max:       70,
				Precision: 1,
			},
		})

		waterLevel := model.MakePropertyByType(model.FloatPropertyType)
		waterLevel.SetRetrievable(true)
		waterLevel.SetParameters(model.FloatPropertyParameters{
			Instance: model.WaterLevelPropertyInstance,
			Unit:     model.UnitPercent,
		})
		co2Level := model.MakePropertyByType(model.FloatPropertyType)
		co2Level.SetRetrievable(true)
		co2Level.SetParameters(model.FloatPropertyParameters{
			Instance: model.CO2LevelPropertyInstance,
			Unit:     model.UnitPPM,
		})
		relativeHumidity := model.MakePropertyByType(model.FloatPropertyType)
		relativeHumidity.SetRetrievable(true)
		relativeHumidity.SetParameters(model.FloatPropertyParameters{
			Instance: model.HumidityPropertyInstance,
			Unit:     model.UnitPercent,
		})
		temperature := model.MakePropertyByType(model.FloatPropertyType)
		temperature.SetRetrievable(true)
		temperature.SetParameters(model.FloatPropertyParameters{
			Instance: model.TemperaturePropertyInstance,
			Unit:     model.UnitTemperatureCelsius,
		})
		amperage := model.MakePropertyByType(model.FloatPropertyType)
		amperage.SetRetrievable(true)
		amperage.SetParameters(model.FloatPropertyParameters{
			Instance: model.AmperagePropertyInstance,
			Unit:     model.UnitAmpere,
		})
		voltage := model.MakePropertyByType(model.FloatPropertyType)
		voltage.SetRetrievable(true)
		voltage.SetParameters(model.FloatPropertyParameters{
			Instance: model.VoltagePropertyInstance,
			Unit:     model.UnitVolt,
		})
		power := model.MakePropertyByType(model.FloatPropertyType)
		power.SetRetrievable(true)
		power.SetParameters(model.FloatPropertyParameters{
			Instance: model.PowerPropertyInstance,
			Unit:     model.UnitWatt,
		})
		pm1density := model.MakePropertyByType(model.FloatPropertyType)
		pm1density.SetRetrievable(true)
		pm1density.SetParameters(model.FloatPropertyParameters{
			Instance: model.PM1DensityPropertyInstance,
			Unit:     model.UnitDensityMcgM3,
		})
		pm2p5density := model.MakePropertyByType(model.FloatPropertyType)
		pm2p5density.SetRetrievable(true)
		pm2p5density.SetParameters(model.FloatPropertyParameters{
			Instance: model.PM2p5DensityPropertyInstance,
			Unit:     model.UnitDensityMcgM3,
		})
		pm10density := model.MakePropertyByType(model.FloatPropertyType)
		pm10density.SetRetrievable(true)
		pm10density.SetParameters(model.FloatPropertyParameters{
			Instance: model.PM10DensityPropertyInstance,
			Unit:     model.UnitDensityMcgM3,
		})
		tvoc := model.MakePropertyByType(model.FloatPropertyType)
		tvoc.SetRetrievable(true)
		tvoc.SetParameters(model.FloatPropertyParameters{
			Instance: model.TvocPropertyInstance,
			Unit:     model.UnitDensityMcgM3,
		})
		batteryLevel := model.MakePropertyByType(model.FloatPropertyType)
		batteryLevel.SetRetrievable(true)
		batteryLevel.SetParameters(model.FloatPropertyParameters{
			Instance: model.BatteryLevelPropertyInstance,
			Unit:     model.UnitPercent,
		})
		pressure := model.MakePropertyByType(model.FloatPropertyType)
		pressure.SetRetrievable(true)
		pressure.SetParameters(model.FloatPropertyParameters{
			Instance: model.PressurePropertyInstance,
			Unit:     model.UnitPressureMmHg,
		})
		timerProperty := model.MakePropertyByType(model.FloatPropertyType)
		timerProperty.SetRetrievable(true)
		timerProperty.SetParameters(model.FloatPropertyParameters{
			Instance: model.TimerPropertyInstance,
			Unit:     model.UnitTimeSeconds,
		})
		smokeConcentration := model.MakePropertyByType(model.FloatPropertyType)
		smokeConcentration.SetRetrievable(true)
		smokeConcentration.SetParameters(model.FloatPropertyParameters{
			Instance: model.SmokeConcentrationPropertyInstance,
			Unit:     model.UnitPercent,
		})
		gasConcentration := model.MakePropertyByType(model.FloatPropertyType)
		gasConcentration.SetRetrievable(true)
		gasConcentration.SetParameters(model.FloatPropertyParameters{
			Instance: model.GasConcentrationPropertyInstance,
			Unit:     model.UnitPercent,
		})
		expectedDevices := []model.Device{
			{
				Name:         "Zhimi Humidifier",
				Aliases:      []string{},
				ExternalID:   "humidifier-external-id-1",
				ExternalName: "Zhimi Humidifier",
				SkillID:      "xiaomi",
				Type:         model.HumidifierDeviceType,
				OriginalType: model.HumidifierDeviceType,
				Capabilities: []model.ICapability{onOff, humidityRange},
				HouseholdID:  currentHousehold.ID,
				Properties: model.Properties{
					waterLevel, co2Level, relativeHumidity, temperature, voltage, amperage, power,
					pm1density, pm2p5density, pm10density, tvoc, pressure, batteryLevel, timerProperty,
					smokeConcentration, gasConcentration,
				},
				DeviceInfo: &model.DeviceInfo{},
				Updated:    discoveryTimestamp,
				Created:    server.dbClient.CurrentTimestamp(),
				Status:     model.UnknownDeviceStatus,
			},
		}

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"humidifier-discovery-1": {
						RequestID: "humidifier-discovery-1",
						Timestamp: discoveryTimestamp,
						Payload: adapter.DiscoveryPayload{
							UserID: strconv.FormatUint(alice.ID, 10),
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "humidifier-external-id-1",
									Name: "Zhimi Humidifier",
									Capabilities: []adapter.CapabilityInfoView{
										{
											Retrievable: true,
											Type:        model.OnOffCapabilityType,
											Parameters:  model.OnOffCapabilityParameters{},
										},
										{
											Retrievable: true,
											Type:        model.RangeCapabilityType,
											Parameters: model.RangeCapabilityParameters{
												Instance:     model.HumidityRangeInstance,
												Unit:         model.UnitPercent,
												RandomAccess: true,
												Looped:       false,
												Range: &model.Range{
													Min:       10,
													Max:       70,
													Precision: 1,
												},
											},
										},
									},
									Properties: []adapter.PropertyInfoView{
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.WaterLevelPropertyInstance,
												Unit:     model.UnitPercent,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.CO2LevelPropertyInstance,
												Unit:     model.UnitPPM,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.HumidityPropertyInstance,
												Unit:     model.UnitPercent,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.TemperaturePropertyInstance,
												Unit:     model.UnitTemperatureCelsius,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.VoltagePropertyInstance,
												Unit:     model.UnitVolt,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.AmperagePropertyInstance,
												Unit:     model.UnitAmpere,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.PowerPropertyInstance,
												Unit:     model.UnitWatt,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.PM1DensityPropertyInstance,
												Unit:     model.UnitDensityMcgM3,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.PM2p5DensityPropertyInstance,
												Unit:     model.UnitDensityMcgM3,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.PM10DensityPropertyInstance,
												Unit:     model.UnitDensityMcgM3,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.TvocPropertyInstance,
												Unit:     model.UnitDensityMcgM3,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.PressurePropertyInstance,
												Unit:     model.UnitPressureMmHg,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.BatteryLevelPropertyInstance,
												Unit:     model.UnitPercent,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.TimerPropertyInstance,
												Unit:     model.UnitTimeSeconds,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.SmokeConcentrationPropertyInstance,
												Unit:     model.UnitPercent,
											},
										},
										{
											Type:        model.FloatPropertyType,
											Retrievable: true,
											Parameters: model.FloatPropertyParameters{
												Instance: model.GasConcentrationPropertyInstance,
												Unit:     model.UnitPercent,
											},
										},
									},
									Type: model.HumidifierDeviceType,
								},
							},
						},
					},
				},
			).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"humidifier-query-1": {
						RequestID: "humidifier-query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: "humidifier-external-id-1",
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.RangeCapabilityType,
											State: model.RangeCapabilityState{
												Instance: model.HumidityRangeInstance,
												Value:    50,
											},
											Timestamp: queryTimestamp1,
										},
									},
									Properties: []adapter.PropertyStateView{
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.HumidityPropertyInstance,
												Value:    40,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TemperaturePropertyInstance,
												Value:    20,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.WaterLevelPropertyInstance,
												Value:    100,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.CO2LevelPropertyInstance,
												Value:    700,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.AmperagePropertyInstance,
												Value:    5,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.VoltagePropertyInstance,
												Value:    10,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PowerPropertyInstance,
												Value:    50,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM1DensityPropertyInstance,
												Value:    1,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM2p5DensityPropertyInstance,
												Value:    2.5,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM10DensityPropertyInstance,
												Value:    10,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TvocPropertyInstance,
												Value:    0.5,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PressurePropertyInstance,
												Value:    700,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.BatteryLevelPropertyInstance,
												Value:    100,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TimerPropertyInstance,
												Value:    15,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.SmokeConcentrationPropertyInstance,
												Value:    4,
											},
											Timestamp: queryTimestamp1,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.GasConcentrationPropertyInstance,
												Value:    4,
											},
											Timestamp: queryTimestamp1,
										},
									},
								},
							},
						},
					},
					"humidifier-query-2": {
						RequestID: "humidifier-query-2",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: "humidifier-external-id-1",
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.RangeCapabilityType,
											State: model.RangeCapabilityState{
												Instance: model.HumidityRangeInstance,
												Value:    60,
											},
											Timestamp: queryTimestamp2,
										},
									},
									Properties: []adapter.PropertyStateView{
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.HumidityPropertyInstance,
												Value:    50,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TemperaturePropertyInstance,
												Value:    15,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.WaterLevelPropertyInstance,
												Value:    50,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.CO2LevelPropertyInstance,
												Value:    600,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.AmperagePropertyInstance,
												Value:    5,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.VoltagePropertyInstance,
												Value:    10,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PowerPropertyInstance,
												Value:    50,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM1DensityPropertyInstance,
												Value:    1,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM2p5DensityPropertyInstance,
												Value:    2.5,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PM10DensityPropertyInstance,
												Value:    10,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TvocPropertyInstance,
												Value:    0.5,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.PressurePropertyInstance,
												Value:    700,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.BatteryLevelPropertyInstance,
												Value:    20,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.TimerPropertyInstance,
												Value:    10,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.SmokeConcentrationPropertyInstance,
												Value:    17,
											},
											Timestamp: queryTimestamp2,
										},
										{
											Type: model.FloatPropertyType,
											State: model.FloatPropertyState{
												Instance: model.GasConcentrationPropertyInstance,
												Value:    17,
											},
											Timestamp: queryTimestamp2,
										},
									},
								},
							},
						},
					},
				},
			).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"humidifier-action-1": {
						RequestID: "humidifier-action-1",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: "humidifier-external-id-1",
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.RangeCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: string(model.HumidityRangeInstance),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
											Timestamp: actionTimestamp,
										},
									},
								},
							},
						},
					},
				},
			)

		discoveryRequest := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("humidifier-discovery-1").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(discoveryRequest)
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "humidifier-external-id-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedDiscoveryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "humidifier-discovery-1",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id": "%s",
					"name": "%s",
					"type": "%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedDiscoveryResponseBody, actualBody)

		queryRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("humidifier-query-1").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "humidifier-query-1",
			"id": "%s",
			"name": "Zhimi Humidifier",
			"names": ["Zhimi Humidifier"],
			"type": "devices.types.humidifier",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [
				{
					"retrievable": true,
					"type": "devices.capabilities.on_off",
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {"split": false}
				},
				{
					"retrievable": true,
					"type": "devices.capabilities.range",
					"state": {
						"instance": "humidity",
						"value": 50
					},
					"parameters": {
						"instance": "humidity",
						"name": "влажность",
						"unit": "unit.percent",
						"random_access": true,
						"looped": false,
						"range": {
							"min": 10,
							"max": 70,
							"precision": 1
						}
					}
				}
			],
			"properties": [
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "water_level",
						"name": "уровень воды",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 100,
						"status": "normal",
						"value": 100
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "co2_level",
						"name": "уровень углекислого газа",
						"unit": "unit.ppm"
					},
					"state": {
						"percent": 50,
						"status": "normal",
						"value": 700
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "humidity",
						"name": "влажность",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 40,
						"status": "normal",
						"value": 40
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "temperature",
						"name": "температура",
						"unit": "unit.temperature.celsius"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 20
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "voltage",
						"name": "текущее напряжение",
						"unit": "unit.volt"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 10
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "amperage",
						"name": "потребление тока",
						"unit": "unit.ampere"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 5
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "power",
						"name": "потребляемая мощность",
						"unit": "unit.watt"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 50
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm1_density",
						"name": "уровень частиц PM1",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 1
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm2.5_density",
						"name": "уровень частиц PM2.5",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 3
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm10_density",
						"name": "уровень частиц PM10",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 10
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "tvoc",
						"name": "уровень органических веществ",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 1
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "battery_level",
						"name": "уровень заряда",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 100,
						"status": "normal",
						"value": 100
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pressure",
						"name": "давление",
						"unit": "unit.pressure.mmhg"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 700
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "timer",
						"name": "таймер",
						"unit": "unit.time.seconds"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 15
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "smoke_concentration",
						"name": "концентрация дыма",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 4,
						"status": "warning",
						"value": 4
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "gas_concentration",
						"name": "концентрация газа",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 4,
						"status": "danger",
						"value": 4
					},
					"last_updated": "1970-01-01T00:31:57Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				}
			],
			"skill_id": "xiaomi",
			"external_id": "humidifier-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))

		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedQueryResponseBody)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		onOff.SetLastUpdated(queryTimestamp1)

		humidityRange.SetState(model.RangeCapabilityState{
			Instance: model.HumidityRangeInstance,
			Relative: nil,
			Value:    50,
		})
		humidityRange.SetLastUpdated(queryTimestamp1)

		waterLevel.SetState(model.FloatPropertyState{
			Instance: model.WaterLevelPropertyInstance,
			Value:    100,
		})
		waterLevel.SetLastUpdated(queryTimestamp1)
		waterLevel.SetStateChangedAt(queryTimestamp1)

		co2Level.SetState(model.FloatPropertyState{
			Instance: model.CO2LevelPropertyInstance,
			Value:    700,
		})
		co2Level.SetLastUpdated(queryTimestamp1)
		co2Level.SetStateChangedAt(queryTimestamp1)

		relativeHumidity.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    40,
		})
		relativeHumidity.SetLastUpdated(queryTimestamp1)
		relativeHumidity.SetStateChangedAt(queryTimestamp1)

		temperature.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		temperature.SetLastUpdated(queryTimestamp1)
		temperature.SetStateChangedAt(queryTimestamp1)

		amperage.SetState(model.FloatPropertyState{
			Instance: model.AmperagePropertyInstance,
			Value:    5,
		})
		amperage.SetLastUpdated(queryTimestamp1)
		amperage.SetStateChangedAt(queryTimestamp1)

		voltage.SetState(model.FloatPropertyState{
			Instance: model.VoltagePropertyInstance,
			Value:    10,
		})
		voltage.SetLastUpdated(queryTimestamp1)
		voltage.SetStateChangedAt(queryTimestamp1)

		power.SetState(model.FloatPropertyState{
			Instance: model.PowerPropertyInstance,
			Value:    50,
		})
		power.SetLastUpdated(queryTimestamp1)
		power.SetStateChangedAt(queryTimestamp1)

		pm1density.SetState(model.FloatPropertyState{
			Instance: model.PM1DensityPropertyInstance,
			Value:    1,
		})
		pm1density.SetLastUpdated(queryTimestamp1)
		pm1density.SetStateChangedAt(queryTimestamp1)

		pm2p5density.SetState(model.FloatPropertyState{
			Instance: model.PM2p5DensityPropertyInstance,
			Value:    2.5,
		})
		pm2p5density.SetLastUpdated(queryTimestamp1)
		pm2p5density.SetStateChangedAt(queryTimestamp1)

		pm10density.SetState(model.FloatPropertyState{
			Instance: model.PM10DensityPropertyInstance,
			Value:    10,
		})
		pm10density.SetLastUpdated(queryTimestamp1)
		pm10density.SetStateChangedAt(queryTimestamp1)

		tvoc.SetState(model.FloatPropertyState{
			Instance: model.TvocPropertyInstance,
			Value:    0.5,
		})
		tvoc.SetLastUpdated(queryTimestamp1)
		tvoc.SetStateChangedAt(queryTimestamp1)

		batteryLevel.SetState(model.FloatPropertyState{
			Instance: model.BatteryLevelPropertyInstance,
			Value:    100,
		})
		batteryLevel.SetLastUpdated(queryTimestamp1)
		batteryLevel.SetStateChangedAt(queryTimestamp1)

		pressure.SetState(model.FloatPropertyState{
			Instance: model.PressurePropertyInstance,
			Value:    700,
		})
		pressure.SetLastUpdated(queryTimestamp1)
		pressure.SetStateChangedAt(queryTimestamp1)

		timerProperty.SetState(model.FloatPropertyState{
			Instance: model.TimerPropertyInstance,
			Value:    15,
		})
		timerProperty.SetLastUpdated(queryTimestamp1)
		timerProperty.SetStateChangedAt(queryTimestamp1)

		smokeConcentration.SetState(model.FloatPropertyState{
			Instance: model.SmokeConcentrationPropertyInstance,
			Value:    4,
		})
		smokeConcentration.SetLastUpdated(queryTimestamp1)
		smokeConcentration.SetStateChangedAt(queryTimestamp1)

		gasConcentration.SetState(model.FloatPropertyState{
			Instance: model.GasConcentrationPropertyInstance,
			Value:    4,
		})
		gasConcentration.SetLastUpdated(queryTimestamp1)
		gasConcentration.SetStateChangedAt(queryTimestamp1)

		expectedDevices = []model.Device{
			{
				Name:         "Zhimi Humidifier",
				Aliases:      []string{},
				ExternalID:   "humidifier-external-id-1",
				ExternalName: "Zhimi Humidifier",
				SkillID:      "xiaomi",
				Type:         model.HumidifierDeviceType,
				OriginalType: model.HumidifierDeviceType,
				Capabilities: []model.ICapability{onOff, humidityRange},
				HouseholdID:  currentHousehold.ID,
				Properties: model.Properties{
					waterLevel, co2Level, relativeHumidity, temperature, voltage, amperage, power,
					pm1density, pm2p5density, pm10density, tvoc, pressure, batteryLevel, timerProperty,
					smokeConcentration, gasConcentration,
				},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       discoveryTimestamp,
				Created:       server.dbClient.CurrentTimestamp(),
				Status:        model.OnlineDeviceStatus,
				StatusUpdated: server.dbClient.CurrentTimestamp(),
			},
		}

		suite.CheckUserDevices(server, &alice.User, expectedDevices)

		actionRequest := newRequest("POST", tools.URLJoin("/m/user/devices/", externalDevice.ID, "actions")).
			withRequestID("humidifier-action-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.RangeCapabilityType,
						"state": JSONObject{
							"instance": model.HumidityRangeInstance,
							"value":    60,
						},
					},
				},
			})
		expectedActionResponseBody := fmt.Sprintf(`{
			"request_id":"humidifier-action-1",
			"status":"ok",
            "devices":[{
                "id":"%s",
			    "capabilities":[{
        			"type":"devices.capabilities.range",
                    "state":{
        				"instance":"humidity",
		        		"action_result":{"status":"DONE"}
				    }
		        }]
            }]
		}`, externalDevice.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)

		humidityRange.SetState(model.RangeCapabilityState{
			Instance: model.HumidityRangeInstance,
			Relative: nil,
			Value:    60,
		})
		humidityRange.SetLastUpdated(actionTimestamp)
		suite.CheckUserDevices(server, &alice.User, expectedDevices)

		queryRequest2 := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("humidifier-query-2").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody2 := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "humidifier-query-2",
			"id": "%s",
			"name": "Zhimi Humidifier",
			"names": ["Zhimi Humidifier"],
			"type": "devices.types.humidifier",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [
				{
					"retrievable": true,
					"type": "devices.capabilities.on_off",
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {"split": false}
				},
				{
					"retrievable": true,
					"type": "devices.capabilities.range",
					"state": {
						"instance": "humidity",
						"value": 60
					},
					"parameters": {
						"instance": "humidity",
						"name": "влажность",
						"unit": "unit.percent",
						"random_access": true,
						"looped": false,
						"range": {
							"min": 10,
							"max": 70,
							"precision": 1
						}
					}
				}
			],
			"properties": [
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "water_level",
						"name": "уровень воды",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 50,
						"status": "warning",
						"value": 50
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "co2_level",
						"name": "уровень углекислого газа",
						"unit": "unit.ppm"
					},
					"state": {
						"percent": 43,
						"status": "normal",
						"value": 600
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "humidity",
						"name": "влажность",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 50,
						"status": "normal",
						"value": 50
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "temperature",
						"name": "температура",
						"unit": "unit.temperature.celsius"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 15
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "voltage",
						"name": "текущее напряжение",
						"unit": "unit.volt"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 10
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "amperage",
						"name": "потребление тока",
						"unit": "unit.ampere"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 5
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "power",
						"name": "потребляемая мощность",
						"unit": "unit.watt"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 50
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm1_density",
						"name": "уровень частиц PM1",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 1
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm2.5_density",
						"name": "уровень частиц PM2.5",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 3
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pm10_density",
						"name": "уровень частиц PM10",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 10
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "tvoc",
						"name": "уровень органических веществ",
						"unit": "unit.density.mcg_m3"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 1
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "battery_level",
						"name": "уровень заряда",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 20,
						"status": "danger",
						"value": 20
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "pressure",
						"name": "давление",
						"unit": "unit.pressure.mmhg"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 700
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:31:57Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "timer",
						"name": "таймер",
						"unit": "unit.time.seconds"
					},
					"state": {
						"percent": null,
						"status": null,
						"value": 10
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "smoke_concentration",
						"name": "концентрация дыма",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 17,
						"status": "danger",
						"value": 17
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				},
				{
                    "type": "devices.properties.float",
					"reportable": false,
					"retrievable": true,
					"parameters": {
						"instance": "gas_concentration",
						"name": "концентрация газа",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 17,
						"status": "danger",
						"value": 17
					},
					"last_updated": "1970-01-01T00:33:20Z",
					"state_changed_at": "1970-01-01T00:33:20Z"
				}
			],
			"skill_id": "xiaomi",
			"external_id": "humidifier-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, queryRequest2, http.StatusOK, expectedQueryResponseBody2)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		onOff.SetLastUpdated(queryTimestamp2)

		humidityRange.SetState(model.RangeCapabilityState{
			Instance: model.HumidityRangeInstance,
			Relative: nil,
			Value:    60,
		})
		humidityRange.SetLastUpdated(queryTimestamp2)

		waterLevel.SetState(model.FloatPropertyState{
			Instance: model.WaterLevelPropertyInstance,
			Value:    50,
		})
		waterLevel.SetLastUpdated(queryTimestamp2)
		waterLevel.SetStateChangedAt(queryTimestamp2)

		co2Level.SetState(model.FloatPropertyState{
			Instance: model.CO2LevelPropertyInstance,
			Value:    600,
		})
		co2Level.SetLastUpdated(queryTimestamp2)
		co2Level.SetStateChangedAt(queryTimestamp2)

		relativeHumidity.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    50,
		})
		relativeHumidity.SetLastUpdated(queryTimestamp2)
		relativeHumidity.SetStateChangedAt(queryTimestamp2)

		temperature.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    15,
		})
		temperature.SetLastUpdated(queryTimestamp2)
		temperature.SetStateChangedAt(queryTimestamp2)

		// Values of these were not changed so we must set only last_updated
		voltage.SetLastUpdated(queryTimestamp2)
		amperage.SetLastUpdated(queryTimestamp2)
		power.SetLastUpdated(queryTimestamp2)
		pm1density.SetLastUpdated(queryTimestamp2)
		pm2p5density.SetLastUpdated(queryTimestamp2)
		pm10density.SetLastUpdated(queryTimestamp2)
		tvoc.SetLastUpdated(queryTimestamp2)
		pressure.SetLastUpdated(queryTimestamp2)

		timerProperty.SetState(model.FloatPropertyState{
			Instance: model.TimerPropertyInstance,
			Value:    10,
		})
		timerProperty.SetLastUpdated(queryTimestamp2)
		timerProperty.SetStateChangedAt(queryTimestamp2)

		batteryLevel.SetState(model.FloatPropertyState{
			Instance: model.BatteryLevelPropertyInstance,
			Value:    20,
		})
		batteryLevel.SetLastUpdated(queryTimestamp2)
		batteryLevel.SetStateChangedAt(queryTimestamp2)

		smokeConcentration.SetState(model.FloatPropertyState{
			Instance: model.SmokeConcentrationPropertyInstance,
			Value:    17,
		})
		smokeConcentration.SetLastUpdated(queryTimestamp2)
		smokeConcentration.SetStateChangedAt(queryTimestamp2)

		gasConcentration.SetState(model.FloatPropertyState{
			Instance: model.GasConcentrationPropertyInstance,
			Value:    17,
		})
		gasConcentration.SetLastUpdated(queryTimestamp2)
		gasConcentration.SetStateChangedAt(queryTimestamp2)

		expectedDevices = []model.Device{
			{
				Name:         "Zhimi Humidifier",
				Aliases:      []string{},
				ExternalID:   "humidifier-external-id-1",
				ExternalName: "Zhimi Humidifier",
				SkillID:      "xiaomi",
				Type:         model.HumidifierDeviceType,
				OriginalType: model.HumidifierDeviceType,
				Capabilities: []model.ICapability{onOff, humidityRange},
				HouseholdID:  currentHousehold.ID,
				Properties: model.Properties{
					waterLevel, co2Level, relativeHumidity, temperature, voltage, amperage, power,
					pm1density, pm2p5density, pm10density, tvoc, pressure, batteryLevel, timerProperty,
					smokeConcentration, gasConcentration,
				},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       discoveryTimestamp,
				Created:       server.dbClient.CurrentTimestamp(),
				Status:        model.OnlineDeviceStatus,
				StatusUpdated: server.dbClient.CurrentTimestamp(),
			},
		}

		suite.CheckUserDevices(server, &alice.User, expectedDevices)
	})

	suite.RunServerTest("TVDeviceIntegration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		var (
			discoveryTimestamp timestamp.PastTimestamp = 1905
			queryTimestamp2    timestamp.PastTimestamp = 2000
		)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(false)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: true})
		expectedDevices := []model.Device{
			{
				Name:         "Mi TV",
				Aliases:      []string{},
				ExternalID:   "tv-external-id-1",
				ExternalName: "Mi TV",
				SkillID:      "xiaomi",
				Type:         model.TvDeviceDeviceType,
				OriginalType: model.TvDeviceDeviceType,
				Capabilities: []model.ICapability{onOff},
				HouseholdID:  currentHousehold.ID,
				Properties:   model.Properties{},
				DeviceInfo:   &model.DeviceInfo{},
				Updated:      discoveryTimestamp,
				Created:      server.dbClient.CurrentTimestamp(),
				Status:       model.UnknownDeviceStatus,
			},
		}

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"tv-discovery-1": {
						RequestID: "tv-discovery-1",
						Timestamp: discoveryTimestamp,
						Payload: adapter.DiscoveryPayload{
							UserID: strconv.FormatUint(alice.ID, 10),
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "tv-external-id-1",
									Name: "Mi TV",
									Type: model.TvDeviceDeviceType,
									Capabilities: []adapter.CapabilityInfoView{
										{
											Retrievable: false,
											Type:        model.OnOffCapabilityType,
											Parameters:  model.OnOffCapabilityParameters{Split: true},
										},
									},
									Properties: []adapter.PropertyInfoView{},
								},
							},
						},
					},
				},
			).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"tv-query-1": {
						RequestID: "tv-query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:        "tv-external-id-1",
									ErrorCode: adapter.DeviceUnreachable,
								},
							},
						},
					},
					"tv-query-2": {
						RequestID: "tv-query-2",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: "tv-external-id-1",
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    false,
											},
											Timestamp: queryTimestamp2,
										},
									},
									Properties: []adapter.PropertyStateView{},
								},
							},
						},
					},
				},
			)

		discoveryRequest := newRequest("POST", "/m/user/skills/xiaomi/discovery").
			withRequestID("tv-discovery-1").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(discoveryRequest)
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "tv-external-id-1", "xiaomi")
		suite.Require().NoError(err, server.Logs())
		expectedDiscoveryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "tv-discovery-1",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id": "%s",
					"name": "%s",
					"type": "%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedDiscoveryResponseBody, actualBody)

		expectedDevices = []model.Device{
			{
				Name:          "Mi TV",
				Aliases:       []string{},
				ExternalID:    "tv-external-id-1",
				ExternalName:  "Mi TV",
				SkillID:       "xiaomi",
				Type:          model.TvDeviceDeviceType,
				OriginalType:  model.TvDeviceDeviceType,
				Capabilities:  []model.ICapability{onOff},
				HouseholdID:   currentHousehold.ID,
				Properties:    model.Properties{},
				DeviceInfo:    &model.DeviceInfo{},
				Updated:       discoveryTimestamp,
				Created:       server.dbClient.CurrentTimestamp(),
				Status:        model.OfflineDeviceStatus,
				StatusUpdated: server.dbClient.CurrentTimestamp(),
			},
		}

		queryRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("tv-query-1").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "tv-query-1",
			"id": "%s",
			"name": "Mi TV",
			"names": ["Mi TV"],
			"type": "devices.types.media_device.tv",
			"icon_url": "%s",
			"state": "offline",
			"groups": [],
			"capabilities": [
				{
					"retrievable": false,
					"type": "devices.capabilities.on_off",
					"state": null,
					"parameters": {"split": true}
				}
			],
			"properties": [],
			"skill_id": "xiaomi",
			"external_id": "tv-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedQueryResponseBody)
		suite.CheckUserDevices(server, &alice.User, expectedDevices)

		queryRequest2 := newRequest("GET", tools.URLJoin("/m/user/devices/", externalDevice.ID)).
			withRequestID("tv-query-2").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody2 := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "tv-query-2",
			"id": "%s",
			"name": "Mi TV",
			"names": ["Mi TV"],
			"type": "devices.types.media_device.tv",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [
				{
					"retrievable": false,
					"type": "devices.capabilities.on_off",
					"state": null,
					"parameters": {"split": true}
				}
			],
			"properties": [],
			"skill_id": "xiaomi",
			"external_id": "tv-external-id-1",
			"favorite": false
		}`, externalDevice.ID, externalDevice.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, queryRequest2, http.StatusOK, expectedQueryResponseBody2)
		suite.CheckUserDevices(server, &alice.User, expectedDevices)
	})
}

func (suite *ServerSuite) TestDeviceCapabilities() {
	suite.RunServerTest("lamp", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		lampColor.SetRetrievable(true)
		lampColor.SetParameters(
			model.ColorSettingCapabilityParameters{
				ColorModel: model.CM(model.RgbModelType),
				ColorSceneParameters: &model.ColorSceneParameters{
					Scenes: model.ColorScenes{
						{
							ID: model.ColorSceneIDParty,
						},
					},
				},
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithCapabilities(
					lampOnOff,
					lampColor,
				),
		)
		suite.Require().NoError(err)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID, "capabilities")).
			withRequestID("capabilities-1").
			withBlackboxUser(&alice.User)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "capabilities-1",
			"id": "%s",
			"name": "Лампочка",
			"type": "",
			"capabilities": [
				{
					"type": "devices.capabilities.on_off",
					"retrievable": true,
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {"split": false}
				},
				{
					"type": "devices.capabilities.color_setting",
					"retrievable": true,
					"state": {
						"instance": "color",
						"value": {
							"id": "red",
							"name": "Красный",
							"type": "multicolor",
							"value": {
								"h": 0,
								"s": 65,
								"v": 100
							}
						}
					},
					"parameters": {
						"instance": "color",
						"name": "цвет",
						"scenes": [
							{
								"id":"party",
								"name":"Вечеринка"
							}
						],
						"palette": [
							{
								"id": "red",
								"name": "Красный",
								"type": "multicolor",
								"value": {
									"h": 0,
									"s": 65,
									"v": 100
								}
							},
							{
								"id": "coral",
								"name": "Коралловый",
								"type": "multicolor",
								"value": {
									"h": 8,
									"s": 55,
									"v": 98
								}
							},
							{
								"id": "orange",
								"name": "Оранжевый",
								"type": "multicolor",
								"value": {
									"h": 25,
									"s": 70,
									"v": 100
								}
							},
							{
								"id": "yellow",
								"name": "Желтый",
								"type": "multicolor",
								"value": {
									"h": 40,
									"s": 70,
									"v": 100
								}
							},
							{
								"id": "lime",
								"name": "Салатовый",
								"type": "multicolor",
								"value": {
									"h": 73,
									"s": 96,
									"v": 100
								}
							},
							{
								"id": "green",
								"name": "Зеленый",
								"type": "multicolor",
								"value": {
									"h": 120,
									"s": 55,
									"v": 90
								}
							},
							{
								"id": "emerald",
								"name": "Изумрудный",
								"type": "multicolor",
								"value": {
									"h": 160,
									"s": 80,
									"v": 90
								}
							},
							{
								"id": "turquoise",
								"name": "Бирюзовый",
								"type": "multicolor",
								"value": {
									"h": 180,
									"s": 80,
									"v": 90
								}
							},
							{
								"id": "cyan",
								"name": "Голубой",
								"type": "multicolor",
								"value": {
									"h": 190,
									"s": 60,
									"v": 100
								}
							},
							{
								"id": "blue",
								"name": "Синий",
								"type": "multicolor",
								"value": {
									"h": 225,
									"s": 55,
									"v": 90
								}
							},
							{
								"id": "moonlight",
								"name": "Лунный",
								"type": "multicolor",
								"value": {
									"h": 231,
									"s": 10,
									"v": 100
								}
							},
							{
								"id": "lavender",
								"name": "Сиреневый",
								"type": "multicolor",
								"value": {
									"h": 255,
									"s": 55,
									"v": 90
								}
							},
							{
								"id": "violet",
								"name": "Фиолетовый",
								"type": "multicolor",
								"value": {
									"h": 270,
									"s": 55,
									"v": 90
								}
							},
							{
								"id": "purple",
								"name": "Пурпурный",
								"type": "multicolor",
								"value": {
									"h": 300,
									"s": 70,
									"v": 90
								}
							},
							{
								"id": "orchid",
								"name": "Розовый",
								"type": "multicolor",
								"value": {
									"h": 305,
									"s": 50,
									"v": 90
								}
							},
							{
								"id": "raspberry",
								"name": "Малиновый",
								"type": "multicolor",
								"value": {
									"h": 345,
									"s": 70,
									"v": 90
								}
							},
							{
								"id": "mauve",
								"name": "Лиловый",
								"type": "multicolor",
								"value": {
									"h": 340,
									"s": 45,
									"v": 90
								}
							}
						]
					}
				}
			]
		}
		`, lamp.ID)

		suite.JSONResponseMatch(server, requestData, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestQuasarProviderQueryMock() {
	suite.RunServerTest("QuasarProviderQuery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").
			WithGroups(model.Group{Name: "Колонки", Type: model.SmartSpeakerDeviceType}).
			WithRooms("Колоночная"))
		suite.Require().NoError(err)

		yandexStation, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Колоночка").
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR).
				WithGroups(alice.Groups["Колонки"]).
				WithRoom(alice.Rooms["Колоночная"]).
				WithCustomData(quasar.CustomData{
					DeviceID: "Королева",
					Platform: "бензоколонки",
				}),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", tools.URLJoin("/m/user/devices/", yandexStation.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		expectedResponseBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"external_id": "%s",
			"name": "%s",
			"names": ["%s"],
			"type": "%s",
			"icon_url": "%s",
			"state": "online",
			"groups": [
				"Колонки"
			],
			"room": "Колоночная",
			"capabilities": [],
            "properties": [],
			"skill_id": "Q",
			"quasar_info": {
				"device_id": "Королева",
				"platform": "бензоколонки",
				"multiroom_available": true,
				"multistep_scenarios_available": true,
                "device_discovery_methods": []
			},
			"favorite": false
		}`, yandexStation.ID, yandexStation.ExternalID, yandexStation.Name, yandexStation.Name, yandexStation.Type, yandexStation.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponseBody)

		request = newRequest("GET", "/m/user/devices/").
			withRequestID("query-2").
			withBlackboxUser(&alice.User)

		expectedStationInfoView := fmt.Sprintf(`
		{
			"id": "%s",
			"name": "%s",
			"type": "%s",
			"item_type": "device",
			"icon_url": "%s",
			"capabilities":[],
			"properties": [],
			"groups": ["Колонки"],
			"skill_id": "%s",
			"quasar_info": {
				"device_id": "Королева",
				"platform": "бензоколонки",
				"multiroom_available": true,
				"multistep_scenarios_available": true,
                "device_discovery_methods": []
			}
		}
		`, yandexStation.ID, yandexStation.Name, yandexStation.Type, yandexStation.Type.IconURL(model.OriginalIconFormat), yandexStation.SkillID)

		expectedGroupInfoView := fmt.Sprintf(`
		{
			"id": "%s",
			"name": "%s",
			"type": "%s",
			"icon_url": "%s",
			"state":"online",
			"capabilities": [],
			"devices_count": 1
		}
		`, alice.Groups["Колонки"].ID, alice.Groups["Колонки"].Name, alice.Groups["Колонки"].Type, alice.Groups["Колонки"].Type.IconURL(model.OriginalIconFormat))
		expectedResponseBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-2",
			"rooms": [
				{
					"id": "%s",
					"name": "Колоночная",
					"devices": [%s]
				}
			],
			"groups": [%s],
			"unconfigured_devices": [],
			"speakers": []
		}`, alice.Rooms["Колоночная"].ID, expectedStationInfoView, expectedGroupInfoView)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponseBody)
	})
}

func (suite *ServerSuite) TestYandexIOProviderQuery() {
	suite.RunServerTest("YandexIOProviderQuery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		yandexStation, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Колоночка").
				WithDeviceType(model.YandexStationMidiDeviceType).
				WithSkillID(model.QUASAR).
				WithCustomData(quasar.CustomData{
					DeviceID: "midi-id-1",
					Platform: "midi",
				}),
		)
		suite.Require().NoError(err)

		yandexIOLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.YANDEXIO).
				WithCapabilities(
					model.MakeCapabilityByType(model.OnOffCapabilityType).
						WithParameters(model.OnOffCapabilityParameters{Split: false}).
						WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true}),
				).
				WithCustomData(yandexiocd.CustomData{ParentEndpointID: yandexStation.ExternalID}),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, model.YANDEXIO, true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"yandexio-query-online": {
						RequestID: "yandexio-query-online",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:           yandexIOLamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{},
									Properties:   []adapter.PropertyStateView{},
								},
							},
						},
					},
					"yandexio-query-offline": {
						RequestID: "yandexio-query-offline",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID:           yandexIOLamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{},
									Properties:   []adapter.PropertyStateView{},
									ErrorCode:    adapter.DeviceUnreachable,
								},
							},
						},
					},
				},
			)
		request := newRequest("GET", tools.URLJoin("/m/user/devices/", yandexIOLamp.ID)).
			withRequestID("yandexio-query-online").
			withBlackboxUser(&alice.User)

		expectedResponseBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "yandexio-query-online",
			"id": "%s",
			"external_id": "%s",
			"name": "%s",
			"names": ["%s"],
			"type": "%s",
			"icon_url": "%s",
			"state": "online",
			"groups": [],
			"capabilities": [
				{
					"retrievable": false,
					"type": "devices.capabilities.on_off",
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {
						"split": false
					}
				}
			],
            "properties": [],
			"skill_id": "YANDEX_IO",
			"favorite": false
		}`, yandexIOLamp.ID, yandexIOLamp.ExternalID, yandexIOLamp.Name, yandexIOLamp.Name, yandexIOLamp.Type, yandexIOLamp.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponseBody)

		request = newRequest("GET", tools.URLJoin("/m/user/devices/", yandexIOLamp.ID)).
			withRequestID("yandexio-query-offline").
			withBlackboxUser(&alice.User)

		expectedResponseBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "yandexio-query-offline",
			"id": "%s",
			"external_id": "%s",
			"name": "%s",
			"names": ["%s"],
			"type": "%s",
			"icon_url": "%s",
			"state": "offline",
			"groups": [],
			"capabilities": [
				{
					"retrievable": false,
					"type": "devices.capabilities.on_off",
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {
						"split": false
					}
				}
			],
            "properties": [],
			"skill_id": "YANDEX_IO",
			"favorite": false
		}`, yandexIOLamp.ID, yandexIOLamp.ExternalID, yandexIOLamp.Name, yandexIOLamp.Name, yandexIOLamp.Type, yandexIOLamp.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponseBody)
	})
}

func (suite *ServerSuite) TestCustomButtonIntegration() {
	suite.RunServerTest("CustomButtonDiscovery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "brand-new-thermostat",
									Name: "Кондиционер",
									Capabilities: []adapter.CapabilityInfoView{
										{
											Retrievable: false,
											Type:        model.CustomButtonCapabilityType,
											Parameters:  model.CustomButtonCapabilityParameters{Instance: "cb_1", InstanceNames: []string{"ТНТ"}},
										},
										{
											Retrievable: true,
											Type:        model.OnOffCapabilityType,
											Parameters:  model.OnOffCapabilityParameters{},
										},
									},
									Type: model.ThermostatDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/T/discovery").
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "brand-new-thermostat", model.TUYA)
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})
}

func (suite *ServerSuite) TestPhraseAndTextActionIntegration() {
	suite.RunServerTest("PhraseAndTextActionDiscovery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&alice.User, model.QUASAR, true).
			WithDiscoveryResponses(
				map[string]adapter.DiscoveryResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.DiscoveryPayload{
							Devices: []adapter.DeviceInfoView{
								{
									ID:   "yandex-station",
									Name: "Колонка",
									Capabilities: []adapter.CapabilityInfoView{
										{
											Retrievable: false,
											Type:        model.QuasarServerActionCapabilityType,
											Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
										},
										{
											Retrievable: true,
											Type:        model.QuasarServerActionCapabilityType,
											Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
										},
									},
									Type: model.YandexStationDeviceType,
								},
							},
						},
					},
				},
			)
		request := newRequest("POST", "/m/user/skills/Q/discovery").
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		externalDevice, err := suite.GetExternalDevice(server, &alice.User, "yandex-station", model.QUASAR)
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [
				{
					"id":"%s",
					"name": "%s",
					"type":"%s"
				}
			]
		}`, externalDevice.ID, externalDevice.Name, externalDevice.Type)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})
}

func (suite *ServerSuite) TestActionRetryIntegration() {
	suite.RunServerTest("ActionRetryIntegration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		toggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
		toggle.SetRetrievable(false)
		toggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})

		modeCap := model.MakeCapabilityByType(model.ModeCapabilityType)
		modeCap.SetRetrievable(true)
		modeCap.SetParameters(model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				model.KnownModes[model.AutoMode],
				model.KnownModes[model.FastMode],
			},
		})

		myDevice, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Мутант").
				WithDeviceType(model.OtherDeviceType).
				WithSkillID(model.XiaomiSkill).
				WithCapabilities(onOff, toggle, modeCap).
				WithExternalID("mutant-id-1"))
		suite.Require().NoError(err, server.Logs())

		providerMock := server.pfMock.NewProvider(&alice.User, model.XiaomiSkill, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: myDevice.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.ToggleCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: toggle.Instance(),
												ActionResult: adapter.StateActionResult{
													Status:    adapter.ERROR,
													ErrorCode: adapter.DeviceUnreachable,
												},
											},
										},
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: onOff.Instance(),
												ActionResult: adapter.StateActionResult{
													Status:    adapter.ERROR,
													ErrorCode: adapter.DeviceUnreachable,
												},
											},
										},
										{
											Type: model.ModeCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: modeCap.Instance(),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
										},
									},
								},
							},
						},
					},
				})
		mobileRequestBody := mobile.ActionRequest{
			Actions: []mobile.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
				},
				{
					Type: model.ToggleCapabilityType,
				},
				{
					Type: model.ModeCapabilityType,
				},
			},
		}
		mobileRequestBody.Actions[0].State.Instance = onOff.Instance()
		mobileRequestBody.Actions[0].State.Value = true
		mobileRequestBody.Actions[1].State.Instance = toggle.Instance()
		mobileRequestBody.Actions[1].State.Value = true
		mobileRequestBody.Actions[2].State.Instance = modeCap.Instance()
		mobileRequestBody.Actions[2].State.Value = model.AutoMode

		request := newRequest("POST", fmt.Sprintf("/m/user/devices/%s/actions", myDevice.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User).
			withBody(mobileRequestBody)
		actualCode, _, actualBody := server.doRequest(request)

		expectedBody := fmt.Sprintf(`
		{
			"status": "error",
			"request_id": "default-req-id",
			"code": "%s",
			"message": "%s"
		}`, provider.ErrorDeviceUnreachable.ErrorCode(), provider.ErrorDeviceUnreachable.MobileErrorMessage())

		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		expectedRequest := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: myDevice.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    true,
									},
								},
								{
									Type: model.ToggleCapabilityType,
									State: model.ToggleCapabilityState{
										Instance: model.PauseToggleCapabilityInstance,
										Value:    true,
									},
								},
								{
									Type: model.ModeCapabilityType,
									State: model.ModeCapabilityState{
										Instance: model.ThermostatModeInstance,
										Value:    model.AutoMode,
									},
								},
							},
						},
					},
				},
			},
		}

		suite.Equal(expectedRequest, providerMock.ActionCalls("default-req-id"), server.Logs())
	})
}

func (suite *ServerSuite) TestActionSocialismTokenNotFound() {
	suite.RunServerTest("ActionSocialismTokenNotFound", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		myDevice, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Однобокий").
				WithDeviceType(model.OtherDeviceType).
				WithSkillID("testProvider").
				WithCapabilities(onOff).
				WithExternalID("mutant-id-1"))
		suite.Require().NoError(err, server.Logs())
		server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithClientError(&socialism.TokenNotFoundError{})

		requestData := newRequest(http.MethodPost, fmt.Sprintf("/m/user/devices/%s/actions", myDevice.ID)).
			withRequestID("action-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.OnOffCapabilityType,
						"state": JSONObject{
							"instance": model.OnOnOffCapabilityInstance,
							"value":    true,
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
		{
			"status": "error",
			"request_id": "action-id",
			"code": "%s",
			"message": "%s"
		}`, model.AccountLinkingError, model.AccountLinkingErrorErrorMessage)

		suite.JSONResponseMatch(server, requestData, 200, expectedBody)
	})
}
