package unlink

import (
	"os"
	"reflect"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/xiva"
)

func (s *unlinkSuite) TestController() {
	s.RunTest("find snatched devices", func(env testEnvironment, c *controller) {
		alice := model.NewUser("alice")
		bob := model.NewUser("bob")
		eve := model.NewUser("eve")
		env.db.InsertUsers(alice, bob, eve)

		snatchSkillID := "goblin-snatch-skill"
		otherSkillID := "other-skill"

		aliceSnatchedLamp := xtestdata.GenerateDevice().WithSkillID(snatchSkillID).WithExternalID("stolen-1")
		aliceSocket := xtestdata.GenerateDevice().WithSkillID(otherSkillID).WithExternalID("stolen-2")
		env.db.InsertDevices(alice, aliceSnatchedLamp, aliceSocket)

		bobSnatchedLamp := xtestdata.GenerateDevice().WithSkillID(snatchSkillID).WithExternalID("stolen-2")
		bobSocket := xtestdata.GenerateDevice().WithSkillID(otherSkillID).WithExternalID("stolen-1")
		env.db.InsertDevices(bob, bobSnatchedLamp, bobSocket)

		// all these devices are belong to eve now
		eveAliceLamp := xtestdata.GenerateDevice().WithSkillID(snatchSkillID).WithExternalID("stolen-1")
		eveBobLamp := xtestdata.GenerateDevice().WithSkillID(snatchSkillID).WithExternalID("stolen-2")
		env.db.InsertDevices(eve, eveAliceLamp, eveBobLamp)

		newEveDevices := xtestdata.WrapDevices(eveAliceLamp, eveBobLamp)
		actualChangedOwnerDevicesMap, err := c.findSnatchedDevicesByExternalID(env.ctx, eve.ID, snatchSkillID, newEveDevices)
		s.Require().NoError(err)

		expectedChangedOwnerDevicesMap := model.DevicesMapByOwnerID{
			alice.ID: {*aliceSnatchedLamp},
			bob.ID:   {*bobSnatchedLamp},
		}

		s.Require().True(reflect.DeepEqual(expectedChangedOwnerDevicesMap, actualChangedOwnerDevicesMap), env.logger.JoinedLogs())
	})

	s.RunTest("delete snatched quasar devices, stereopairs and connected yandexio devices", func(env testEnvironment, c *controller) {
		alice := model.NewUser("alice")
		eve := model.NewUser("eve")
		env.db.InsertUsers(alice, eve)

		aliceSnatchedSpeaker := xtestdata.GenerateDevice().WithSkillID(model.QUASAR).WithExternalID("stolen-quasar-1").WithDeviceType(model.YandexStationMidiDeviceType).WithCustomData(quasar.CustomData{DeviceID: "stolen-quasar-1"})
		aliceOtherSpeaker := xtestdata.GenerateDevice().WithSkillID(model.QUASAR).WithExternalID("other-quasar-1").WithDeviceType(model.YandexStationMidiDeviceType).WithCustomData(quasar.CustomData{DeviceID: "other-quasar-1"})
		aliceSnatchedLamp := xtestdata.GenerateDevice().WithSkillID(model.YANDEXIO).WithExternalID("stolen-yandexio-1").WithCustomData(yandexiocd.CustomData{ParentEndpointID: "stolen-quasar-1"})
		aliceSnatchedSocket := xtestdata.GenerateDevice().WithSkillID(model.YANDEXIO).WithExternalID("stolen-yandexio-2").WithCustomData(yandexiocd.CustomData{ParentEndpointID: "stolen-quasar-1"})
		env.db.InsertDevices(alice, aliceSnatchedSpeaker, aliceOtherSpeaker, aliceSnatchedLamp, aliceSnatchedSocket)
		aliceStereopair := xtestdata.CreateStereopair(xtestdata.WrapDevices(aliceSnatchedSpeaker, aliceOtherSpeaker))
		env.db.InsertStereopairs(alice, aliceStereopair)

		// all these devices are belong to eve now
		eveAliceSpeaker := xtestdata.GenerateDevice().WithSkillID(model.QUASAR).WithExternalID("stolen-quasar-1")
		env.db.InsertDevices(eve, eveAliceSpeaker)

		err := c.DeleteChangedOwnerDevices(env.ctx, eve.ID, model.QUASAR, xtestdata.WrapDevices(eveAliceSpeaker))
		s.Require().NoError(err)

		env.db.AssertDevicesNotPresent(alice, aliceSnatchedSpeaker.ID, aliceSnatchedLamp.ID, aliceSnatchedSocket.ID)
		env.db.AssertStereopairsNotPresent(alice, aliceStereopair.ID)
		env.db.AssertDevicesPresent(alice, aliceOtherSpeaker.ID)
		env.db.AssertDevicesPresent(eve, eveAliceSpeaker.ID)

		err = env.xivaMock.AssertEvent(time.Second, func(event xiva.MockSentEvent) error {
			s.Require().EqualValues(updates.UpdateDeviceListEventID, event.EventID)
			s.Require().Equal(alice.ID, event.UserID)
			return nil
		})
		s.Require().NoError(err)
	})

	s.RunTest("delete snatched yandex io devices", func(env testEnvironment, c *controller) {
		alice := model.NewUser("alice")
		eve := model.NewUser("eve")
		env.db.InsertUsers(alice, eve)

		aliceSnatchedLamp := xtestdata.GenerateDevice().WithSkillID(model.YANDEXIO).WithExternalID("stolen-yandexio-1")
		aliceNormalSocket := xtestdata.GenerateDevice().WithSkillID(model.YANDEXIO).WithExternalID("normal-yandexio-2")
		env.db.InsertDevices(alice, aliceSnatchedLamp, aliceNormalSocket)

		// all these devices are belong to eve now
		eveAliceLamp := xtestdata.GenerateDevice().WithSkillID(model.YANDEXIO).WithExternalID("stolen-yandexio-1")
		env.db.InsertDevices(eve, eveAliceLamp)

		err := c.DeleteChangedOwnerDevices(env.ctx, eve.ID, model.YANDEXIO, xtestdata.WrapDevices(eveAliceLamp))
		s.Require().NoError(err)

		env.db.AssertDevicesNotPresent(alice, aliceSnatchedLamp.ID)
		env.db.AssertDevicesPresent(alice, aliceNormalSocket.ID)
		env.db.AssertDevicesPresent(eve, eveAliceLamp.ID)

		err = env.xivaMock.AssertEvent(time.Second, func(event xiva.MockSentEvent) error {
			s.Require().EqualValues(updates.UpdateDeviceListEventID, event.EventID)
			s.Require().Equal(alice.ID, event.UserID)
			return nil
		})
		s.Require().NoError(err)
	})

	s.RunTest("snatched tuya devices should not be deleted", func(env testEnvironment, c *controller) {
		alice := model.NewUser("alice")
		eve := model.NewUser("eve")
		env.db.InsertUsers(alice, eve)

		aliceSnatchedLamp := xtestdata.GenerateDevice().WithSkillID(model.TUYA).WithExternalID("stolen-tuya-1")
		env.db.InsertDevices(alice, aliceSnatchedLamp)

		// all these devices are belong to eve now
		eveAliceLamp := xtestdata.GenerateDevice().WithSkillID(model.TUYA).WithExternalID("stolen-tuya-1")
		env.db.InsertDevices(eve, eveAliceLamp)

		err := c.DeleteChangedOwnerDevices(env.ctx, eve.ID, model.TUYA, xtestdata.WrapDevices(eveAliceLamp))
		s.Require().NoError(err)

		env.db.AssertDevicesPresent(alice, aliceSnatchedLamp.ID)
		env.db.AssertDevicesPresent(eve, eveAliceLamp.ID)
	})
}

func TestUnlinkController(t *testing.T) {
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic("can not read YDB_ENDPOINT envvar")
	}

	prefix, ok := os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic("can not read YDB_ENDPOINT envvar")
	}

	token, ok := os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	suite.Run(t, &unlinkSuite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	})
}
