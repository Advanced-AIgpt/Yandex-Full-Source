package db

import (
	"math/rand"

	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
)

func (s *Suite) TestUser() {
	s.Run("GetTuyaUserID", func() {
		s.RunDBTest("Success", func(client *TestingDBClient) {
			randomUser := s.randomTuyaUser()
			err := client.CreateUser(client.ctx, randomUser.id, randomUser.skillID, randomUser.login, randomUser.tuyaUID)
			s.NoError(err, client.Logs())

			expectedUserID := randomUser.tuyaUID
			actualUserID, err := client.GetTuyaUserID(client.ctx, randomUser.id, randomUser.skillID)
			s.NoError(err, client.Logs())
			s.Equal(expectedUserID, actualUserID)
		})
		s.RunDBTest("UnknownUser", func(client *TestingDBClient) {
			_, err := client.GetTuyaUserID(client.ctx, rand.Uint64(), testing.RandString(5))
			s.Error(err, client.Logs())
		})
	})
	s.Run("IsKnown", func() {
		s.RunDBTest("Known", func(client *TestingDBClient) {
			randomUser := s.randomTuyaUser()
			err := client.CreateUser(client.ctx, randomUser.id, "skill-id-stub", randomUser.login, randomUser.tuyaUID)
			s.NoError(err, client.Logs())

			actualKnown, err := client.IsKnownUser(client.ctx, randomUser.tuyaUID)
			s.NoError(err, client.Logs())
			s.True(actualKnown)
		})
		s.RunDBTest("Unknown", func(client *TestingDBClient) {
			actualKnown, err := client.IsKnownUser(client.ctx, "random-tuya-user")
			s.NoError(err, client.Logs())
			s.False(actualKnown)
		})
	})
}
