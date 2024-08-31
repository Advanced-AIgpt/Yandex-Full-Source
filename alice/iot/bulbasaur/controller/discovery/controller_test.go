package discovery

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/ptr"
)

func (s *Suite) TestDiscovery() {
	s.RunControllerTest("PropertyWithUnknownTypeIsFiltered", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		externalUserID := "chinese-alice"

		requestID := "default-req-id"
		container.providerFactoryMock.NewProvider(&alice.User, model.XiaomiSkill, true).WithDiscoveryResponses(
			map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: container.timestamperMock.CurrentTimestamp(),
					Payload: adapter.DiscoveryPayload{
						UserID: externalUserID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:           "some-external-device-id-1",
								Name:         "device-with-unknown-properties",
								Type:         model.HumidifierDeviceType,
								Capabilities: []adapter.CapabilityInfoView{},
								Properties: []adapter.PropertyInfoView{
									{
										Type:        "devices.properties.unknown_property_type",
										Reportable:  true,
										Retrievable: false,
										Parameters:  nil,
									},
									{
										Type:        model.FloatPropertyType,
										Reportable:  true,
										Retrievable: true,
										Parameters: model.FloatPropertyParameters{
											Instance: model.TemperaturePropertyInstance,
											Unit:     model.UnitTemperatureCelsius,
										},
									},
								},
							},
						},
					},
				},
			})

		ctx = requestid.WithRequestID(ctx, requestID)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)
		devices, err := container.controller.ProviderDiscovery(ctx, origin, model.XiaomiSkill)
		s.Require().NoError(err)

		s.Len(devices, 1)
		expected := []model.Device{
			{
				Name:         "Увлажнитель",
				ExternalID:   "some-external-device-id-1",
				ExternalName: "Увлажнитель",
				SkillID:      model.XiaomiSkill,
				Type:         model.HumidifierDeviceType,
				OriginalType: model.HumidifierDeviceType,
				Capabilities: []model.ICapability{},
				Properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithReportable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.TemperaturePropertyInstance,
							Unit:     model.UnitTemperatureCelsius,
						}),
				},
				Updated: container.timestamperMock.CurrentTimestamp(),
			},
		}
		s.ElementsMatch(expected, devices)

		devConsoleExpectedMessage := fmt.Sprintf(
			"property devices.properties.unknown_property_type for device some-external-device-id-1 from provider %s validation failed, will skip it",
			model.XiaomiSkill,
		)
		s.NotEmpty(container.logs.FilterMessage(devConsoleExpectedMessage).All())
	})

	s.RunControllerTest("DeviceWithOnlyOneUnknownTypePropertyIsFiltered", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		externalUserID := "chinese-alice"

		requestID := "default-req-id"
		container.providerFactoryMock.NewProvider(&alice.User, model.XiaomiSkill, true).WithDiscoveryResponses(
			map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: container.timestamperMock.CurrentTimestamp(),
					Payload: adapter.DiscoveryPayload{
						UserID: externalUserID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:           "some-external-device-id-1",
								Name:         "device-with-unknown-properties",
								Type:         model.HumidifierDeviceType,
								Capabilities: []adapter.CapabilityInfoView{},
								Properties: []adapter.PropertyInfoView{
									{
										Type:        "devices.properties.unknown_property_type",
										Reportable:  true,
										Retrievable: false,
										Parameters:  nil,
									},
								},
							},
						},
					},
				},
			})

		ctx = requestid.WithRequestID(ctx, requestID)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)
		devices, err := container.controller.ProviderDiscovery(ctx, origin, model.XiaomiSkill)
		s.Require().NoError(err)

		s.Empty(devices)

		devConsoleExpectedMessage := fmt.Sprintf(
			"device some-external-device-id-1 from provider %s validation failed, will skip it: %s",
			model.XiaomiSkill,
			"device must contain at least one property or capability",
		)
		s.NotEmpty(container.logs.FilterMessage(devConsoleExpectedMessage).All())
	})

	s.RunControllerTest("AsyncStereopairDeletion", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		bob, err := dbfiller.InsertUser(ctx, model.NewUser("bob"))
		s.Require().NoError(err)

		// generate stereopair
		generatedLeaderSpeaker := xtestdata.GenerateMidiSpeaker("midi1", "ext-midi", "midi1")
		clonedLeaderSpeaker := generatedLeaderSpeaker.Clone()
		leaderSpeaker, err := dbfiller.InsertDevice(ctx, &alice.User, generatedLeaderSpeaker)
		s.Require().NoError(err)
		followerSpeaker := xtestdata.GenerateMidiSpeaker("midi2", "ext-midi2", "midi2")
		followerSpeaker, err = dbfiller.InsertDevice(ctx, &alice.User, followerSpeaker)
		s.Require().NoError(err)
		stereopair := xtestdata.CreateStereopair(model.Devices{*leaderSpeaker, *followerSpeaker})
		err = container.controller.Database.StoreStereopair(ctx, alice.ID, *stereopair)
		s.Require().NoError(err)
		bobLeaderSpeaker, err := dbfiller.InsertDevice(ctx, &bob.User, &clonedLeaderSpeaker)
		s.Require().NoError(err)

		skillInfo := provider.SkillInfo{
			HumanReadableName: model.HumanReadableQuasarProviderName,
			SkillID:           model.QUASAR,
			Trusted:           true,
		}
		discoveryResult := adapter.DiscoveryResult{
			RequestID: "request-id",
			Timestamp: container.timestamperMock.CurrentTimestamp(),
			Payload: adapter.DiscoveryPayload{
				UserID: "bob-ext-id",
				Devices: []adapter.DeviceInfoView{
					{
						ID:           generatedLeaderSpeaker.ExternalID,
						Name:         generatedLeaderSpeaker.Name,
						Type:         generatedLeaderSpeaker.Type,
						CustomData:   generatedLeaderSpeaker.CustomData,
						Capabilities: []adapter.CapabilityInfoView{},
						Properties:   []adapter.PropertyInfoView{},
					},
				},
			},
		}
		container.controller.postprocessingAsync(
			ctx,
			model.Origin{User: model.User{ID: bob.ID}},
			skillInfo,
			model.Devices{*bobLeaderSpeaker},
			discoveryResult,
		)
		time.Sleep(time.Second * 10)
		// no stereopair
		_, err = container.controller.Database.SelectStereopair(ctx, alice.ID, stereopair.ID)
		s.Error(err)

		// no leader speaker
		_, err = container.controller.Database.SelectUserDevice(ctx, alice.ID, leaderSpeaker.ID)
		s.Error(err)
		s.ErrorIs(err, &model.DeviceNotFoundError{})
	})
}

func (s *discoverySuite) TestDiscovery() {
	s.RunTest("PushDiscovery", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)
		env.db.InsertExternalUsers("alice-external", model.XiaomiSkill, alice.User)

		origin := model.NewOrigin(env.ctx, model.SearchAppSurfaceParameters{}, alice.User)

		discoveryResult := adapter.DiscoveryResult{
			RequestID: "push-discovery-reqid",
			Timestamp: 2086,
			Payload: adapter.DiscoveryPayload{
				UserID: "alice-external",
				Devices: []adapter.DeviceInfoView{
					{
						ID:           "lamp-external-id",
						Name:         "Лампа",
						Type:         model.LightDeviceType,
						Capabilities: []adapter.CapabilityInfoView{xtestadapter.OnOffCapability()},
						Properties:   []adapter.PropertyInfoView{},
					},
				},
			},
		}
		env.pf.NewProvider(&alice.User, model.XiaomiSkill, true)

		_, err := c.PushDiscovery(env.ctx, model.XiaomiSkill, origin, discoveryResult)
		s.Require().NoError(err)

		currentHousehold, err := env.db.DBClient().SelectCurrentHousehold(env.ctx, alice.ID)
		s.Require().NoError(err)

		lamp := xtestdata.GenerateLamp("", "lamp-external-id", model.XiaomiSkill).WithHouseholdID(currentHousehold.ID).WithUpdated(2086).WithCreated(1)
		lamp.Capabilities = model.Capabilities{xtestdata.OnOffCapability(false).WithState((*model.OnOffCapabilityState)(nil))}
		env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(lamp))

		// lets update device info on that device and check it
		deviceInfo := &model.DeviceInfo{
			Manufacturer: ptr.String("LUMI"),
			Model:        ptr.String("LUMI-LIGHT-100500"),
		}
		discoveryResult.Payload.Devices[0].DeviceInfo = deviceInfo

		_, err = c.PushDiscovery(env.ctx, model.XiaomiSkill, origin, discoveryResult)
		s.Require().NoError(err)

		lamp.DeviceInfo = deviceInfo
		env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(lamp))
	})
}
