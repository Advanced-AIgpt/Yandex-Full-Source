package api

import (
	"encoding/json"
	"github.com/stretchr/testify/assert"
	"testing"
)

var testRequest = []byte(`{
  "meta": {
    "client_id": "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
    "interfaces": {
      "account_linking": {},
      "payments": {},
      "screen": {}
    },
    "locale": "ru-RU",
    "timezone": "UTC"
  },
  "request": {
    "command": "1 апреля в Москве -1, солнечно",
    "nlu": {
      "entities": [
        {
          "tokens": {
            "end": 1,
            "start": 0
          },
          "type": "YANDEX.NUMBER",
          "value": 1
        },
        {
          "tokens": {
            "end": 2,
            "start": 0
          },
          "type": "YANDEX.DATETIME",
          "value": {"day": 1,"day_is_relative": false,"month": 4,"month_is_relative": false}
        },
        {
          "tokens": {
            "end": 4,
            "start": 2
          },
          "type": "YANDEX.GEO",
          "value": {"city": "москва"}
        },
        {
          "tokens": {
            "end": 5,
            "start": 4
          },
          "type": "YANDEX.NUMBER",
          "value": -1
        }
      ],
      "tokens": [
        "1",
        "апреля",
        "в",
        "москве",
        "1",
        "солнечно"
      ]
    },
    "original_utterance": "1 апреля в Москве -1, солнечно",
    "type": "SimpleUtterance"
  },
  "session": {
    "message_id": 3,
    "new": false,
    "session_id": "57e097b7-a414a1d0-a93bcce4-a9ed26b1",
    "skill_id": "b3b79017-42bf-4fac-b40c-061914390741",
    "user_id": "061E4B999551D74C54F2BC1FE75A2AC7A01DA484D396D13D03E8ABA3FE3FA194"
  },
  "version": "1.0"
}`)

var request = Request{
	Meta: Meta{
		Locale:   "ru-RU",
		Timezone: "UTC",
		ClientID: "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
		Interfaces: Interfaces{
			Screen: &struct{}{},
		},
	},
	Request: RequestBody{

		Command:           "1 апреля в Москве -1, солнечно",
		OriginalUtterance: "1 апреля в Москве -1, солнечно",
		Type:              "SimpleUtterance",
		Nlu: &Nlu{
			Tokens: []string{
				"1",
				"апреля",
				"в",
				"москве",
				"1",
				"солнечно",
			},
			Entities: []Entity{
				{
					Tokens: EntityTokens{0, 1},
					Type:   "YANDEX.NUMBER",
					Value:  []byte(`1`),
				},
				{
					Tokens: EntityTokens{0, 2},
					Type:   "YANDEX.DATETIME",
					Value:  []byte(`{"day": 1,"day_is_relative": false,"month": 4,"month_is_relative": false}`),
				},
				{
					Tokens: EntityTokens{2, 4},
					Type:   "YANDEX.GEO",
					Value:  []byte(`{"city": "москва"}`),
				},
				{
					Tokens: EntityTokens{4, 5},
					Type:   "YANDEX.NUMBER",
					Value:  []byte(`-1`),
				},
			},
		},
	},
	Session: Session{
		New:       false,
		MessageID: 3,
		SessionID: "57e097b7-a414a1d0-a93bcce4-a9ed26b1",
		UserID:    "061E4B999551D74C54F2BC1FE75A2AC7A01DA484D396D13D03E8ABA3FE3FA194",
		SkillID:   "b3b79017-42bf-4fac-b40c-061914390741",
	},
	Version: "1.0",
}

func TestRequestNlu(t *testing.T) {
	var r Request
	assert.NoError(t, json.Unmarshal(testRequest, &r))
	assert.Equal(t, request, r)
}
