package server

import (
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (suite *ServerSuite) TestUserTakeout() {
	suite.RunServerTest("TestUserTakeout", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		// devices
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		brightness := model.MakeCapabilityByType(model.RangeCapabilityType)
		brightness.SetRetrievable(true)
		brightness.SetParameters(
			model.RangeCapabilityParameters{
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
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithGroups(alice.Groups["Люстра"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(
					onOff,
					brightness,
				),
		)
		suite.Require().NoError(err)

		// scenarios
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Сценарий").
				WithIcon(model.ScenarioIconDay).
				WithDevices(
					model.ScenarioDevice{
						ID: lamp.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err)

		// networks
		_, err = dbfiller.InsertNetwork(server.ctx, &alice.User,
			model.NewNetwork("my-shiny-network"))
		suite.Require().NoError(err)

		household, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err)

		request := newRequest("POST", "/takeout").
			withRequestID("takeout-1").
			withBlackboxUser(&alice.User).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: takeoutTvmID,
			}).withBody(RawBodyString(fmt.Sprintf(`uid=%s`, strconv.FormatUint(alice.ID, 10))))

		devicesStr := fmt.Sprintf(`"devices":{"%s":{"id":"%s","name":"Лампа","type":"devices.types.light","household_id":"%s","room_id":"%s","group_ids":["%s"]}}`,
			lamp.ID, lamp.ID, household.ID, alice.Rooms["Кухня"].ID, alice.Groups["Люстра"].ID)
		groupsStr := fmt.Sprintf(`"groups":{"%s":{"id":"%s","name":"Люстра","household_id":"%s"}}`,
			alice.Groups["Люстра"].ID, alice.Groups["Люстра"].ID, household.ID)
		roomsStr := fmt.Sprintf(`"rooms":{"%s":{"id":"%s","name":"Кухня","household_id":"%s"}}`,
			alice.Rooms["Кухня"].ID, alice.Rooms["Кухня"].ID, household.ID)
		scenarioStr := fmt.Sprintf(`"scenarios":{"%s":{"id":"%s","name":"Сценарий","affected_device_ids":["%s"],"requested_actions":[]}}`,
			scenario.ID, scenario.ID, lamp.ID)
		householdStr := fmt.Sprintf(`"households":{"%s":{"id":"%s","name":"%s"}}`,
			household.ID, household.ID, household.Name)
		networksStr := `"networks":[{"SSID":"my-shiny-network"}]`
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"data": {
				"data.json": "{%s,%s,%s,%s,%s,%s}"
			}
		}`, strings.ReplaceAll(devicesStr, `"`, `\"`),
			strings.ReplaceAll(groupsStr, `"`, `\"`),
			strings.ReplaceAll(roomsStr, `"`, `\"`),
			strings.ReplaceAll(scenarioStr, `"`, `\"`),
			strings.ReplaceAll(householdStr, `"`, `\"`),
			strings.ReplaceAll(networksStr, `"`, `\"`))

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

}
