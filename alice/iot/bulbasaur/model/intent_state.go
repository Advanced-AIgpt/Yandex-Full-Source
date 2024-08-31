package model

import (
	"encoding/json"
)

type IntentState json.RawMessage

type IntentStateKey struct {
	SpeakerID string
	SessionID string
	Intent    string
}

func NewIntentStateKey(speakerID string, sessionID string, intent string) IntentStateKey {
	return IntentStateKey{
		SpeakerID: speakerID,
		SessionID: sessionID,
		Intent:    intent,
	}
}
