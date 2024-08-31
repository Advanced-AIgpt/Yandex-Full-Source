package callback

import (
	"context"
	"github.com/stretchr/testify/assert"
	"os"
	"strconv"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *callbackSuite) TestCallbackDiscovery() {
	const skillID = "MEGAtestingSKILL"
	const requestID = "def-req-id"
	const goroutineWaitDuration = time.Second * 2

	s.RunControllerTest("new_capabilities_and_device_info_on_socket", func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler) {
		const aliceExternalID = "alice-super-external-test-1"
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err, c.Logs())

		currentHousehold, err := c.db.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err, c.Logs())

		err = c.db.StoreExternalUser(ctx, aliceExternalID, skillID, alice.User)
		s.Require().NoError(err, c.Logs())

		// provider made device info and added toggle capability
		c.pfMock.NewProvider(&alice.User, skillID, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "super-socket-1337",
								Name: "Розетка",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Retrievable: true,
										Type:        model.OnOffCapabilityType,
										Parameters:  model.OnOffCapabilityParameters{},
									},
									{
										Retrievable: true,
										Type:        model.ToggleCapabilityType,
										Parameters:  model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance},
									},
								},
								Properties: []adapter.PropertyInfoView{},
								Type:       model.SocketDeviceType,
								DeviceInfo: &model.DeviceInfo{
									Manufacturer: tools.AOS("Yandex"),
									Model:        tools.AOS("SUPER_SOCKET"),
									HwVersion:    tools.AOS("1.0"),
								},
							},
						},
					},
				},
			})

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		_, err = dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("socket").
			WithExternalID("super-socket-1337").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.SocketDeviceType),
		)
		s.Require().NoError(err, c.Logs())

		payload := callback.DiscoveryPayload{
			Filter:         callback.DiscoveryDefaultFilter{},
			ExternalUserID: aliceExternalID,
		}

		// try to update all sockets of this user
		err = c.CallbackDiscovery(requestid.WithRequestID(ctx, requestID), skillID, payload)
		s.Require().NoError(err, c.Logs())

		time.Sleep(goroutineWaitDuration)

		toggleCap := model.MakeCapabilityByType(model.ToggleCapabilityType)
		toggleCap.SetRetrievable(true)
		toggleCap.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		expected := []model.Device{
			{
				Name:         "socket",
				Aliases:      []string{},
				ExternalID:   "super-socket-1337",
				ExternalName: "Розетка",
				SkillID:      skillID,
				Type:         model.SocketDeviceType,
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					onOff,
					toggleCap,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo: &model.DeviceInfo{
					Manufacturer: tools.AOS("Yandex"),
					Model:        tools.AOS("SUPER_SOCKET"),
					HwVersion:    tools.AOS("1.0"),
				},
				Updated: timestamp.CurrentTimestampMock,
				Created: timestamp.CurrentTimestampMock,
				Status:  model.UnknownDeviceStatus,
			},
		}

		s.CheckUserDevices(ctx, c, alice.ID, expected)
	})
	s.RunControllerTest("filter_unnecessary_devices", func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler) {
		const aliceExternalID = "alice-super-external-test-2"

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err, c.Logs())

		currentHousehold, err := c.db.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err, c.Logs())

		err = c.db.StoreExternalUser(ctx, aliceExternalID, skillID, alice.User)
		s.Require().NoError(err, c.Logs())

		// provider made device info and added toggle capability to socket
		// also he has lamp but we do not want to discover it
		c.pfMock.NewProvider(&alice.User, skillID, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "super-socket-1337",
								Name: "Розетка",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Retrievable: true,
										Type:        model.OnOffCapabilityType,
										Parameters:  model.OnOffCapabilityParameters{},
									},
									{
										Retrievable: true,
										Type:        model.ToggleCapabilityType,
										Parameters:  model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance},
									},
								},
								Properties: []adapter.PropertyInfoView{},
								Type:       model.SocketDeviceType,
								DeviceInfo: &model.DeviceInfo{
									Manufacturer: tools.AOS("Yandex"),
									Model:        tools.AOS("SUPER_SOCKET"),
									HwVersion:    tools.AOS("1.0"),
								},
							},
							{
								ID:   "super-lamp-1337",
								Name: "Лампочка",
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
								Properties: []adapter.PropertyInfoView{},
								Type:       model.LightDeviceType,
							},
						},
					},
				},
			})

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		_, err = dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("socket").
			WithExternalID("super-socket-1337").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.SocketDeviceType),
		)
		s.Require().NoError(err, c.Logs())

		payload := callback.DiscoveryPayload{
			Filter:         callback.DiscoveryDeviceTypeFilter{DeviceTypes: []string{string(model.HubDeviceType)}},
			FilterType:     callback.DiscoveryDeviceTypeFilterType,
			ExternalUserID: aliceExternalID,
		}

		// try to update all hubs of this user but provider does not have hubs
		err = c.CallbackDiscovery(requestid.WithRequestID(ctx, requestID), skillID, payload)
		s.Require().NoError(err, c.Logs())

		time.Sleep(goroutineWaitDuration)

		expected := []model.Device{
			{
				Name:         "socket",
				Aliases:      []string{},
				ExternalID:   "super-socket-1337",
				ExternalName: "socket",
				SkillID:      skillID,
				Type:         model.SocketDeviceType,
				OriginalType: model.OtherDeviceType,
				Capabilities: []model.ICapability{
					onOff,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo:  &model.DeviceInfo{},
				Created:     c.dbClient.CurrentTimestamp(),
				Status:      model.UnknownDeviceStatus,
			},
		}

		s.CheckUserDevices(ctx, c, alice.ID, expected)
	})
	s.RunControllerTest("usual_callback_discovery", func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler) {
		const aliceExternalID = "alice-super-external-test-3"

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err, c.Logs())

		currentHousehold, err := c.db.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err, c.Logs())

		err = c.db.StoreExternalUser(ctx, aliceExternalID, skillID, alice.User)
		s.Require().NoError(err, c.Logs())

		// provider made device info and added toggle capability to socket
		// also he has lamp
		c.pfMock.NewProvider(&alice.User, skillID, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "super-socket-1337",
								Name: "Розетка",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Retrievable: true,
										Type:        model.OnOffCapabilityType,
										Parameters:  model.OnOffCapabilityParameters{},
									},
									{
										Retrievable: true,
										Type:        model.ToggleCapabilityType,
										Parameters:  model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance},
									},
								},
								Properties: []adapter.PropertyInfoView{},
								Type:       model.SocketDeviceType,
								DeviceInfo: &model.DeviceInfo{
									Manufacturer: tools.AOS("Yandex"),
									Model:        tools.AOS("SUPER_SOCKET"),
									HwVersion:    tools.AOS("1.0"),
								},
							},
							{
								ID:   "super-lamp-1337",
								Name: "Лампочка",
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
								Properties: []adapter.PropertyInfoView{},
								Type:       model.LightDeviceType,
							},
						},
					},
				},
			})

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		_, err = dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("socket").
			WithExternalID("super-socket-1337").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.SocketDeviceType),
		)
		s.Require().NoError(err, c.Logs())

		// try to update all devices

		payload := callback.DiscoveryPayload{
			Filter:         callback.DiscoveryDefaultFilter{},
			ExternalUserID: aliceExternalID,
		}
		err = c.CallbackDiscovery(requestid.WithRequestID(ctx, requestID), skillID, payload)
		s.Require().NoError(err, c.Logs())

		time.Sleep(goroutineWaitDuration)

		toggleCap := model.MakeCapabilityByType(model.ToggleCapabilityType)
		toggleCap.SetRetrievable(true)
		toggleCap.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		rangeCap := model.MakeCapabilityByType(model.RangeCapabilityType)
		rangeCap.SetRetrievable(true)
		rangeCap.SetParameters(model.RangeCapabilityParameters{
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

		expected := []model.Device{
			{
				Name:         "socket",
				Aliases:      []string{},
				ExternalID:   "super-socket-1337",
				ExternalName: "Розетка",
				SkillID:      skillID,
				Type:         model.SocketDeviceType,
				OriginalType: model.OtherDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: model.Capabilities{
					onOff,
					toggleCap,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{
					Manufacturer: tools.AOS("Yandex"),
					Model:        tools.AOS("SUPER_SOCKET"),
					HwVersion:    tools.AOS("1.0"),
				},
				Updated: timestamp.CurrentTimestampMock,
				Created: timestamp.CurrentTimestampMock,
				Status:  model.UnknownDeviceStatus,
			},
			{
				Name:         "Лампочка",
				Aliases:      []string{},
				ExternalID:   "super-lamp-1337",
				ExternalName: "Лампочка",
				SkillID:      skillID,
				Type:         model.LightDeviceType,
				OriginalType: model.LightDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: model.Capabilities{
					onOff,
					rangeCap,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{},
				Updated:    timestamp.CurrentTimestampMock,
				Created:    timestamp.CurrentTimestampMock,
				Status:     model.UnknownDeviceStatus,
			},
		}

		s.CheckUserDevices(ctx, c, alice.ID, expected)
	})

	s.RunControllerTest("do_not_discover_archived_devices", func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler) {
		const aliceExternalID = "alice-super-external-test-4"
		const archivedLampRequestID = "archived-lamp-request-id"

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err, c.Logs())
		origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, alice.User)

		currentHousehold, err := c.db.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err, c.Logs())

		err = c.db.StoreExternalUser(ctx, aliceExternalID, skillID, alice.User)
		s.Require().NoError(err, c.Logs())

		// provider made device info and added toggle capability to socket
		// also he has lamp, that was previously archived by user
		// also he has lamp that was archived and rediscovered by user later
		c.pfMock.NewProvider(&alice.User, skillID, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "super-socket-1337",
								Name: "Розетка",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Retrievable: true,
										Type:        model.OnOffCapabilityType,
										Parameters:  model.OnOffCapabilityParameters{},
									},
									{
										Retrievable: true,
										Type:        model.ToggleCapabilityType,
										Parameters:  model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance},
									},
								},
								Properties: []adapter.PropertyInfoView{},
								Type:       model.SocketDeviceType,
								DeviceInfo: &model.DeviceInfo{
									Manufacturer: tools.AOS("Yandex"),
									Model:        tools.AOS("SUPER_SOCKET"),
									HwVersion:    tools.AOS("1.0"),
								},
							},
							{
								ID:   "super-lamp-1337",
								Name: "Лампочка",
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
								Properties: []adapter.PropertyInfoView{},
								Type:       model.LightDeviceType,
							},
							{
								ID:   "super-lamp-1337-ar",
								Name: "Лампочка",
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
								Properties: []adapter.PropertyInfoView{},
								Type:       model.LightDeviceType,
							},
						},
					},
				},
				archivedLampRequestID: {
					RequestID: archivedLampRequestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "super-lamp-1337-ar",
								Name: "Лампочка",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Retrievable: true,
										Type:        model.OnOffCapabilityType,
										Parameters:  model.OnOffCapabilityParameters{},
									},
								},
								Properties: []adapter.PropertyInfoView{},
								Type:       model.LightDeviceType,
							},
						},
					},
				},
			})

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		_, err = dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("socket").
			WithExternalID("super-socket-1337").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.SocketDeviceType),
		)
		s.Require().NoError(err, c.Logs())

		lamp, err := dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("lamp").
			WithExternalID("super-lamp-1337").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.LightDeviceType),
		)
		s.Require().NoError(err, c.Logs())

		existLamp, err := dbfiller.InsertDevice(ctx, &alice.User, model.NewDevice("lamp").
			WithExternalID("super-lamp-1337-ar").
			WithSkillID(skillID).
			WithCapabilities(onOff).
			WithDeviceType(model.LightDeviceType),
		)

		s.Require().NoError(err, c.Logs())

		// move exist lamp to archived
		err = c.dbClient.DeleteUserDevices(ctx, alice.ID, []string{existLamp.ID})
		s.Require().NoError(err, c.Logs())

		// rediscover it
		archivedLampCtx := requestid.WithRequestID(ctx, archivedLampRequestID)
		archivedLampDiscoveredDevices, err := c.discoveryController.ProviderDiscovery(archivedLampCtx, origin, skillID)
		s.Require().NoError(err, c.Logs())
		_, err = c.discoveryController.StoreDiscoveredDevices(archivedLampCtx, alice.User, archivedLampDiscoveredDevices)
		s.Require().NoError(err, c.Logs())

		// move lamp to archived
		err = c.dbClient.DeleteUserDevices(ctx, alice.ID, []string{lamp.ID})
		s.Require().NoError(err, c.Logs())

		// try to update all devices
		payload := callback.DiscoveryPayload{
			Filter:         callback.DiscoveryDefaultFilter{},
			ExternalUserID: aliceExternalID,
		}
		err = c.CallbackDiscovery(requestid.WithRequestID(ctx, requestID), skillID, payload)
		s.Require().NoError(err, c.Logs())

		time.Sleep(goroutineWaitDuration)

		toggleCap := model.MakeCapabilityByType(model.ToggleCapabilityType)
		toggleCap.SetRetrievable(true)
		toggleCap.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		rangeCap := model.MakeCapabilityByType(model.RangeCapabilityType)
		rangeCap.SetRetrievable(true)
		rangeCap.SetParameters(model.RangeCapabilityParameters{
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

		expected := []model.Device{
			{
				Name:         "socket",
				Aliases:      []string{},
				ExternalID:   "super-socket-1337",
				ExternalName: "Розетка",
				SkillID:      skillID,
				Type:         model.SocketDeviceType,
				OriginalType: model.OtherDeviceType,
				Capabilities: model.Capabilities{
					onOff,
					toggleCap,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo: &model.DeviceInfo{
					Manufacturer: tools.AOS("Yandex"),
					Model:        tools.AOS("SUPER_SOCKET"),
					HwVersion:    tools.AOS("1.0"),
				},
				Updated: timestamp.CurrentTimestampMock,
				Created: timestamp.CurrentTimestampMock,
				Status:  model.UnknownDeviceStatus,
			},
			{
				Name:         "Лампочка",
				Aliases:      []string{},
				ExternalID:   "super-lamp-1337-ar",
				ExternalName: "Лампочка",
				SkillID:      skillID,
				Type:         model.LightDeviceType,
				OriginalType: model.LightDeviceType,
				Capabilities: model.Capabilities{
					onOff,
					rangeCap,
				},
				HouseholdID: currentHousehold.ID,
				Properties:  model.Properties{},
				DeviceInfo:  &model.DeviceInfo{},
				Updated:     timestamp.CurrentTimestampMock,
				Created:     timestamp.CurrentTimestampMock,
				Status:      model.UnknownDeviceStatus,
			},
		}

		s.CheckUserDevices(ctx, c, alice.ID, expected)
	})

	s.RunControllerTest("remove_station_for_previous_owner", func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler) {
		bob, err := dbfiller.InsertUser(ctx, model.NewUser("bob"))
		s.Require().NoError(err, c.Logs())

		yandexSpeaker, err := dbfiller.InsertDevice(ctx, &bob.User,
			model.
				NewDevice("Станция").
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR).
				WithExternalID("yandex-station"))
		s.Require().NoError(err, c.Logs())

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err, c.Logs())
		aliceCurrentHousehold, err := c.db.SelectCurrentHousehold(ctx, alice.ID)
		s.Require().NoError(err, c.Logs())

		var aliceExternalID = strconv.FormatUint(alice.ID, 10)
		err = c.db.StoreExternalUser(ctx, aliceExternalID, model.QUASAR, alice.User)
		s.Require().NoError(err, c.Logs())

		c.pfMock.NewProvider(&alice.User, model.QUASAR, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				requestID: {
					RequestID: requestID,
					Timestamp: timestamp.CurrentTimestampMock,
					Payload: adapter.DiscoveryPayload{
						UserID: aliceExternalID,
						Devices: []adapter.DeviceInfoView{
							{
								ID:           yandexSpeaker.ExternalID,
								Name:         "Станция",
								Capabilities: []adapter.CapabilityInfoView{},
								Properties:   []adapter.PropertyInfoView{},
								Type:         model.YandexStationDeviceType,
							},
						},
					},
				},
			})

		payload := callback.DiscoveryPayload{
			Filter:         callback.DiscoveryDefaultFilter{},
			ExternalUserID: aliceExternalID,
		}
		err = c.CallbackDiscovery(requestid.WithRequestID(ctx, requestID), model.QUASAR, payload)
		s.Require().NoError(err, c.Logs())

		time.Sleep(goroutineWaitDuration)

		expected := []model.Device{
			{
				Name:         yandexSpeaker.Name,
				Aliases:      []string{},
				ExternalID:   yandexSpeaker.ExternalID,
				ExternalName: yandexSpeaker.ExternalName,
				SkillID:      model.QUASAR,
				Type:         model.YandexStationDeviceType,
				OriginalType: model.YandexStationDeviceType,
				Capabilities: model.Capabilities{},
				HouseholdID:  aliceCurrentHousehold.ID,
				Properties:   model.Properties{},
				DeviceInfo:   &model.DeviceInfo{},
				Updated:      timestamp.CurrentTimestampMock,
				Created:      timestamp.CurrentTimestampMock,
				Status:       model.UnknownDeviceStatus,
			},
		}
		s.CheckUserDevices(ctx, c, alice.ID, expected)
		s.CheckUserDevices(ctx, c, bob.ID, nil)
	})
}

func TestCallbackController(t *testing.T) {
	var endpoint, prefix, token string

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(xerrors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(xerrors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	credentials := dbCredentials{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	}

	suite.Run(t, &callbackSuite{
		dbCredentials:        credentials,
		historyDBCredentials: credentials,
	})
}

func TestFlappingFilter(t *testing.T) {
	openProperty := model.MakePropertyByType(model.EventPropertyType).
		WithParameters(model.EventPropertyParameters{
			Instance: model.OpenPropertyInstance,
		}).
		WithState(
			model.EventPropertyState{
				Instance: model.OpenPropertyInstance,
				Value:    model.OpenedEvent,
			}).WithLastUpdated(1657555442.953)

	clickProperty := model.MakePropertyByType(model.EventPropertyType).
		WithParameters(model.EventPropertyParameters{
			Instance: model.ButtonPropertyInstance,
		}).
		WithState(
			model.EventPropertyState{
				Instance: model.OpenPropertyInstance,
				Value:    model.OpenedEvent,
			}).WithLastUpdated(1657555442.100)

	newProperties := model.Properties{
		openProperty,
		clickProperty,
	}

	existingProperties := model.Properties{
		(openProperty.Clone()).(*model.EventProperty).WithLastUpdated(1657555443.623),
		(clickProperty.Clone()).(*model.EventProperty).WithLastUpdated(1657555440.000),
	}

	callbackController := Controller{logger: &nop.Logger{}}
	filteredProperties := callbackController.filterFlappingPropertiesForHistory(context.Background(), existingProperties, newProperties)
	assert.Equal(t, model.Properties{clickProperty}, filteredProperties)
}
