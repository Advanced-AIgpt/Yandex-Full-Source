package db

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) TestIntentStates() {
	user := data.GenerateUser()

	type testIntentState struct {
		Key string `json:"key"`
	}
	state := testIntentState{Key: "My-Value"}
	intentStatekey := model.IntentStateKey{
		SpeakerID: "speaker-id",
		SessionID: "session-id",
		Intent:    "discovery",
	}
	_, err := s.dbClient.SelectUserIntentState(s.context, user.ID, intentStatekey)
	s.True(xerrors.Is(err, &model.IntentStateNotFoundError{}))

	rawMessage, err := json.Marshal(state)
	s.NoError(err)

	// store state in db
	err = s.dbClient.StoreUserIntentState(s.context, user.ID, intentStatekey, rawMessage)
	s.NoError(err)

	// check valid select work
	rawSelectedState, err := s.dbClient.SelectUserIntentState(s.context, user.ID, intentStatekey)
	s.NoError(err)
	s.NotNil(rawSelectedState)

	selectedState := testIntentState{}

	err = json.Unmarshal(rawSelectedState, &selectedState)
	s.NoError(err)
	s.Equal(state, selectedState)
}
