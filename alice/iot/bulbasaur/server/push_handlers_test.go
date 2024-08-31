package server

import (
	"fmt"
	"net/http"
	"strconv"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/xiva"
)

func (suite *ServerSuite) TestPushDiscovery() {
	const RequestID string = "def-req-id"
	const skillID string = "some-skill"

	suite.RunServerTest("pushDiscoveryHandler", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		err = server.server.db.StoreExternalUser(server.ctx, "alice-external-2002", skillID, alice.User)
		suite.Require().NoError(err, server.Logs())

		discoveryResult := JSONObject{
			"request_id": RequestID,
			"ts":         server.timestamper.CurrentTimestamp(),
			"payload": JSONObject{
				"user_id": "alice-external-2002",
				"devices": JSONArray{
					JSONObject{
						"id":   "some-lamp",
						"name": "Лампочка",
						"capabilities": JSONArray{
							JSONObject{
								"retrievable": true,
								"type":        model.OnOffCapabilityType,
								"parameters":  model.OnOffCapabilityParameters{},
							},
							JSONObject{
								"retrievable": true,
								"type":        model.RangeCapabilityType,
								"parameters": model.RangeCapabilityParameters{
									Instance:     model.BrightnessRangeInstance,
									Unit:         model.UnitPercent,
									RandomAccess: true,
									Looped:       false,
									Range: &model.Range{
										Min:       1,
										Max:       100,
										Precision: 1,
									},
								},
							},
						},
						"device_info": JSONObject{},
						"properties":  JSONArray{},
						"type":        model.LightDeviceType,
					},
				},
			},
		}
		server.pfMock.NewProvider(&alice.User, skillID, true)

		request := newRequest(http.MethodPost, fmt.Sprintf("/v1.0/push/skills/%s/discovery", skillID)).withTvmData(&tvmData{
			srcServiceID: steelixTvmID,
		}).withBody(discoveryResult).withRequestID(RequestID)

		expectedBody := fmt.Sprintf(`{"request_id":"%s","status":"ok"}`, RequestID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		brightness := model.MakeCapabilityByType(model.RangeCapabilityType)
		brightness.SetRetrievable(true)
		brightness.SetParameters(model.RangeCapabilityParameters{
			Instance:     model.BrightnessRangeInstance,
			Unit:         model.UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &model.Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		})

		lamp := model.Device{
			Name:         "Лампочка",
			Aliases:      []string{},
			ExternalID:   "some-lamp",
			ExternalName: "Лампочка",
			SkillID:      skillID,
			Type:         model.LightDeviceType,
			OriginalType: model.LightDeviceType,
			Capabilities: []model.ICapability{onOff, brightness},
			HouseholdID:  currentHousehold.ID,
			Properties:   []model.IProperty{},
			DeviceInfo:   &model.DeviceInfo{},
			Updated:      server.timestamper.CurrentTimestamp(),
			Created:      server.timestamper.CurrentTimestamp(),
			Status:       model.UnknownDeviceStatus,
		}

		suite.CheckUserDevices(server, &alice.User, []model.Device{lamp})
		suite.Equal(1, server.sup.PushCount(alice.ID))

		suite.NoError(server.xiva.AssertEvent(250*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
			suite.Equal(alice.User.ID, actualEvent.UserID)
			suite.EqualValues(updates.UpdateDeviceListEventID, actualEvent.EventID)

			event := actualEvent.EventData.Payload.(updates.UpdateDeviceListEvent)
			suite.Equal(updates.DiscoverySource, event.Source)
			return nil
		}))
	})

	suite.RunServerTest("pushDiscoveryHandlerSupPushesDoNotReactOnQuasar", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		stringifiedAliceID := strconv.FormatUint(alice.ID, 10)
		err = server.server.db.StoreExternalUser(server.ctx, stringifiedAliceID, model.QUASAR, alice.User)
		suite.Require().NoError(err, server.Logs())

		discoveryResult := JSONObject{
			"request_id": RequestID,
			"ts":         server.timestamper.CurrentTimestamp(),
			"payload": JSONObject{
				"user_id": stringifiedAliceID,
				"devices": JSONArray{
					JSONObject{
						"id":           "speaker",
						"name":         "Колонка",
						"capabilities": JSONArray{},
						"device_info":  JSONObject{},
						"properties":   JSONArray{},
						"type":         model.YandexStationDeviceType,
					},
				},
			},
		}
		server.pfMock.NewProvider(&alice.User, model.QUASAR, true)

		request := newRequest(http.MethodPost, fmt.Sprintf("/v1.0/push/skills/%s/discovery", model.QUASAR)).withTvmData(&tvmData{
			srcServiceID: steelixTvmID,
		}).withBody(discoveryResult).withRequestID(RequestID)

		expectedBody := fmt.Sprintf(`{"request_id":"%s","status":"ok"}`, RequestID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		phraseCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseCap.SetRetrievable(false)
		phraseCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		textCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textCap.SetRetrievable(false)
		textCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

		speakerCapabilities := model.Capabilities{textCap, phraseCap}
		for _, knownInstance := range model.KnownQuasarCapabilityInstances {
			capability := model.MakeCapabilityByType(model.QuasarCapabilityType)
			capability.SetRetrievable(false)
			capability.SetParameters(model.QuasarCapabilityParameters{Instance: model.QuasarCapabilityInstance(knownInstance)})
			speakerCapabilities = append(speakerCapabilities, capability)
		}

		speaker := model.Device{
			Name:         "Колонка",
			Aliases:      []string{},
			ExternalID:   "speaker",
			ExternalName: "Колонка",
			SkillID:      model.QUASAR,
			Type:         model.YandexStationDeviceType,
			OriginalType: model.YandexStationDeviceType,
			HouseholdID:  currentHousehold.ID,
			Capabilities: speakerCapabilities,
			Properties:   []model.IProperty{},
			DeviceInfo:   &model.DeviceInfo{},
			Updated:      server.timestamper.CurrentTimestamp(),
			Created:      server.timestamper.CurrentTimestamp(),
			Status:       model.UnknownDeviceStatus,
		}

		suite.CheckUserDevices(server, &alice.User, []model.Device{speaker})
		suite.Equal(0, server.sup.PushCount(alice.ID))
	})
}
