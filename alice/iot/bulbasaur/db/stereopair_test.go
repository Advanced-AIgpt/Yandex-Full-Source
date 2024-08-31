package db

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (s *DBClientSuite) TestStereopair() {
	at := assert.New(s.T())
	ctx := s.context

	user1 := data.GenerateUser()

	err := s.dbClient.StoreUser(ctx, user1)
	at.NoError(err)

	user2 := data.GenerateUser()
	err = s.dbClient.StoreUser(ctx, user2)
	at.NoError(err)

	userid1 := user1.ID
	userid2 := user2.ID

	newStationDevice := func(user model.User, suffix string) (model.Device, error) {
		device := *model.NewDevice("name " + suffix)
		device.ID = "id-" + suffix
		device.Type = model.YandexStationDeviceType
		device.SkillID = model.QUASAR
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(ctx, user, device)
		at.NoError(err)
		at.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(ctx, storedDevice)

		return device, err
	}

	device111, err := newStationDevice(user1, "1-1-1")
	at.NoError(err)

	device112, err := newStationDevice(user1, "1-1-2")
	at.NoError(err)

	device121, err := newStationDevice(user1, "1-2-1")
	at.NoError(err)

	device122, err := newStationDevice(user1, "1-2-2")
	at.NoError(err)

	device131, err := newStationDevice(user1, "1-3-1")
	at.NoError(err)

	device132, err := newStationDevice(user1, "1-3-2")
	at.NoError(err)

	device211, err := newStationDevice(user2, "2-1-1")
	at.NoError(err)

	device212, err := newStationDevice(user2, "2-1-2")
	at.NoError(err)

	stereopairs := map[uint64]model.Stereopairs{
		userid1: {
			{
				ID:   "stereo-1-1",
				Name: "name-1-1",
				Config: model.StereopairConfig{Devices: model.StereopairDeviceConfigs{
					{
						ID:      device111.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					{
						ID:      device112.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				}},
				Devices: model.Devices{device111, device112},
			},
			{
				ID:   "stereo-1-2",
				Name: "name-1-2",
				Config: model.StereopairConfig{
					Devices: model.StereopairDeviceConfigs{
						{
							ID:      device121.ID,
							Channel: model.RightChannel,
							Role:    model.LeaderRole,
						},
						{
							ID:      device122.ID,
							Channel: model.LeftChannel,
							Role:    model.FollowerRole,
						},
					},
				},
				Devices: model.Devices{device121, device122},
			},
			{
				ID:   "stereo-1-3",
				Name: "name-1-3",
				Config: model.StereopairConfig{
					Devices: model.StereopairDeviceConfigs{
						{
							ID:      device131.ID,
							Channel: model.RightChannel,
							Role:    model.FollowerRole,
						},
						{
							ID:      device132.ID,
							Channel: model.LeftChannel,
							Role:    model.LeaderRole,
						},
					},
				},
				Devices: model.Devices{device131, device132},
			},
		},
		userid2: {{
			ID:   "stereo-2-1",
			Name: "name-1-4",
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					{
						ID:      device211.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
					{
						ID:      device212.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
				},
			},
			Devices: model.Devices{device211, device212},
		}},
	}

	for key, userStereopairs := range stereopairs {
		for i := range userStereopairs {
			sort.Sort(model.DevicesSorting(userStereopairs[i].Devices))
		}
		stereopairs[key] = userStereopairs
	}

	// check store
	for userid, pairs := range stereopairs {
		for _, pair := range pairs {
			at.NoError(s.dbClient.StoreStereopair(ctx, userid, pair), "save pair: %+v", pair)
		}
	}

	// check load

	// user1
	selectedPairs, err := s.dbClient.SelectStereopairs(ctx, userid1)
	at.NoError(err)
	sort.Sort(selectedPairs)
	at.Equal(stereopairs[userid1], selectedPairs)

	// user2
	selectedPairs, err = s.dbClient.SelectStereopairs(ctx, userid2)
	at.NoError(err)
	sort.Sort(selectedPairs)
	at.Equal(stereopairs[userid2], selectedPairs)

	// delete
	at.NoError(s.dbClient.deleteStereopairs(ctx, userid1, []string{"stereo-1-2"}))

	expected := make(model.Stereopairs, 0, 2)
	for _, pair := range stereopairs[userid1] {
		if pair.ID == "stereo-1-2" {
			continue
		}
		expected = append(expected, pair)
	}
	selectedPairs, err = s.dbClient.SelectStereopairs(ctx, userid1)
	at.NoError(err)
	sort.Sort(selectedPairs)
	at.Equal(expected, selectedPairs)
}

func (s *DBClientSuite) TestDeviceAsStereopairPart() {
	ctx := s.context
	at := assert.New(s.T())

	user := data.GenerateUser()
	err := s.dbClient.StoreUser(ctx, user)
	at.NoError(err)

	speakers := model.Devices{data.GenerateDevice(), data.GenerateDevice(), data.GenerateDevice()}
	for i := range speakers {
		speaker := &speakers[i]
		speaker.Type = model.YandexStationDeviceType
		storedSpeaker, _, err := s.dbClient.StoreUserDevice(ctx, user, *speaker)
		at.NoError(err)
		speaker.ID = storedSpeaker.ID
	}

	leader := speakers[0]
	follower := speakers[1]
	noStereopairPart := speakers[2]
	stereopair := model.Stereopair{
		ID:   leader.ID,
		Name: "тестовая пара",
		Config: model.StereopairConfig{Devices: model.StereopairDeviceConfigs{
			{
				ID:      leader.ID,
				Channel: model.LeftChannel,
				Role:    model.LeaderRole,
			},
			{
				ID:      follower.ID,
				Channel: model.RightChannel,
				Role:    model.FollowerRole,
			},
		}},
	}

	err = s.dbClient.StoreStereopair(ctx, user.ID, stereopair)
	at.NoError(err)

	stereopairs, err := s.dbClient.SelectStereopairs(ctx, user.ID)
	at.NoError(err)

	at.Equal(model.LeaderRole, stereopairs.GetDeviceRole(leader.ID))
	at.Equal(model.FollowerRole, stereopairs.GetDeviceRole(follower.ID))
	at.Equal(model.NoStereopair, stereopairs.GetDeviceRole(noStereopairPart.ID))
}
