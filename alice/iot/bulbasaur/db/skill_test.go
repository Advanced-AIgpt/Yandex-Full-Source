package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
)

func (s *DBClientSuite) TestUserSkills() {
	s.Run("unknown user", func() {
		alice := data.GenerateUser()
		skills, err := s.dbClient.SelectUserSkills(s.context, alice.ID)
		s.Require().NoError(err)
		s.Equal(len(skills), 0)
	})

	s.Run("SelectAndStoreSkills", func() {
		alice := data.GenerateUser()
		expectedSkills := []string{
			"wow such good skill",
			"wow such bad skill",
		}

		err := s.dbClient.StoreUserSkill(s.context, alice.ID, expectedSkills[0])
		s.Require().NoError(err)
		err = s.dbClient.StoreUserSkill(s.context, alice.ID, expectedSkills[1])
		s.Require().NoError(err)

		actualSkills, err := s.dbClient.SelectUserSkills(s.context, alice.ID)
		s.Require().NoError(err)
		s.ElementsMatch(expectedSkills, actualSkills)

		exist, err := s.dbClient.CheckUserSkillExist(s.context, alice.ID, expectedSkills[0])
		s.Require().NoError(err)
		s.True(exist)

		exist, err = s.dbClient.CheckUserSkillExist(s.context, alice.ID, "unexpected skill")
		s.Require().NoError(err)
		s.False(exist)
	})

	s.Run("DeleteUserSkill", func() {
		alice := data.GenerateUser()
		expectedSkills := []string{
			"awesome skill",
		}

		err := s.dbClient.StoreUserSkill(s.context, alice.ID, expectedSkills[0])
		s.Require().NoError(err)

		actualSkills, err := s.dbClient.SelectUserSkills(s.context, alice.ID)
		s.Require().NoError(err)
		s.Equal(actualSkills, expectedSkills)

		err = s.dbClient.DeleteUserSkill(s.context, alice.ID, expectedSkills[0])
		s.Require().NoError(err)

		actualSkills, err = s.dbClient.SelectUserSkills(s.context, alice.ID)
		s.Require().NoError(err)
		s.Equal(len(actualSkills), 0)

		exist, err := s.dbClient.CheckUserSkillExist(s.context, alice.ID, expectedSkills[0])
		s.Require().NoError(err)
		s.False(exist)
	})
}
