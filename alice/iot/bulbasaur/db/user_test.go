package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) Test_User() {
	var (
		actualUser model.User
		err        error
	)

	user := data.GenerateUser()

	userWithChanges := user
	userWithChanges.Login = "updated@login"

	// 1. Unknown user
	_, err = s.dbClient.SelectUser(s.context, user.ID)
	s.True(xerrors.Is(err, &model.UnknownUserError{}))

	// 2. Store user
	err = s.dbClient.StoreUser(s.context, user)
	s.NoError(err)

	households, err := s.dbClient.SelectUserHouseholds(s.context, user.ID)
	s.Equal(1, len(households))
	s.NoError(err)

	currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	s.NoError(err)
	s.Equal(households[0], currentHousehold)

	actualUser, err = s.dbClient.SelectUser(s.context, user.ID)
	s.NoError(err)
	s.Equal(user, actualUser)

	// 3. User will not be updated
	err = s.dbClient.StoreUser(s.context, userWithChanges)
	s.NoError(err)

	actualUser, err = s.dbClient.SelectUser(s.context, user.ID)
	s.NoError(err)
	s.Equal(user, actualUser)

	// 4. Corner cases
	err = s.dbClient.StoreUser(s.context, model.User{})
	s.NoError(err)
}

func (s *DBClientSuite) Test_ExternalUser() {
	// when reading these examples, know this
	// one yandex uid, one skill id -> one external id
	// one external id -> many yandex uids!

	alice := data.GenerateUser()
	alice.Login = "" // we need only uids in these tests

	// xiaomi requests to save externalAlice as alice
	err := s.dbClient.StoreExternalUser(s.context, "externalAlice", "xiaomi", alice)
	s.NoError(err)

	// assert that alice is now stored in db
	users, err := s.dbClient.SelectExternalUsers(s.context, "externalAlice", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{alice}, users)

	// xiaomi now requests to save alice as externalYaFamily
	err = s.dbClient.StoreExternalUser(s.context, "externalYaFamily", "xiaomi", alice)
	s.NoError(err)

	// assert that alice is stored
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalYaFamily", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{alice}, users)

	// we can store only one externalUser for our yandex uid -> expect overwrite behaviour
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalAlice", "xiaomi")
	s.NoError(err)
	s.Equal([]model.User{}, users) // i find this important. users are empty, but not nil

	// xiaomi now stores another family member with externalYaFamily id
	bob := data.GenerateUser()
	bob.Login = "" // we need only uids in these tests
	err = s.dbClient.StoreExternalUser(s.context, "externalYaFamily", "xiaomi", bob)
	s.NoError(err)

	// assert that we can select the whole family with this externalYaFamily id
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalYaFamily", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{alice, bob}, users)

	// let's store eve and assert that we don't select her when search for <xiaomi, externalYaFamily>
	eve := data.GenerateUser()
	eve.Login = ""
	err = s.dbClient.StoreExternalUser(s.context, "externalEve", "xiaomi", eve)
	s.NoError(err)

	// assert that eve is not selected
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalYaFamily", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{alice, bob}, users)

	// assert that eve exists in ExternalUsers
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalEve", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{eve}, users)

	// alice left bob. xiaomi unlinks her from their account
	err = s.dbClient.DeleteExternalUser(s.context, "xiaomi", alice)
	s.NoError(err)

	// assert that only bob is left in externalYaFamily
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalYaFamily", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{bob}, users)

	// assert that eve still exists in ExternalUsers (who knows what can happen)
	users, err = s.dbClient.SelectExternalUsers(s.context, "externalEve", "xiaomi")
	s.NoError(err)
	s.ElementsMatch([]model.User{eve}, users)
}
