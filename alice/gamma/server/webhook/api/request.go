package api

// Webhook protocol: https://tech.yandex.ru/dialogs/alice/doc/protocol-docpage/

import (
	"bytes"
	"encoding/json"
	"strings"

	"github.com/golang/protobuf/jsonpb"
	structpb "github.com/golang/protobuf/ptypes/struct"
	"golang.org/x/xerrors"

	skillsAPI "a.yandex-team.ru/alice/gamma/sdk/api"
)

type Meta struct {
	Locale      string                 `json:"locale"`
	Timezone    string                 `json:"timezone"`
	ClientID    string                 `json:"client_id"`
	Interfaces  Interfaces             `json:"interfaces"`
	Experiments map[string]interface{} `json:"experiments,omitempty"`
}

type Interfaces struct {
	Screen *struct{} `json:"screen"`
}

type EntityTokens struct {
	Start int64 `json:"start"`
	End   int64 `json:"end"`
}

type Entity struct {
	Tokens EntityTokens    `json:"tokens"`
	Type   string          `json:"type"`
	Value  json.RawMessage `json:"value"`
}

type Nlu struct {
	Tokens   []string `json:"tokens"`
	Entities []Entity `json:"entities"`
}

type RequestBody struct {
	Command           string          `json:"command"`
	OriginalUtterance string          `json:"original_utterance"`
	Type              string          `json:"type"`
	Payload           json.RawMessage `json:"payload"`
	Nlu               *Nlu            `json:"nlu"`
}

type Session struct {
	New       bool   `json:"new"`
	MessageID int64  `json:"message_id"`
	SessionID string `json:"session_id"`
	UserID    string `json:"user_id"`
	SkillID   string `json:"skill_id"`
}

type Request struct {
	Meta      Meta        `json:"meta"`
	Request   RequestBody `json:"request"`
	Session   Session     `json:"session"`
	Version   string      `json:"version"`
	RequestID string      `json:"request_id"`
	State     State       `json:"state"`
}
type State struct {
	SessionState json.RawMessage `json:"session"`
	UserState    json.RawMessage `json:"user"`
}

func newValue(message json.RawMessage) (*structpb.Value, error) {
	var res structpb.Value
	if err := jsonpb.Unmarshal(bytes.NewReader(message), &res); err != nil {
		return nil, err
	}
	return &res, nil
}

func nluFromRequest(nlu *Nlu) (*skillsAPI.Nlu, error) {
	result := &skillsAPI.Nlu{}
	if nlu == nil {
		return result, nil
	}

	result.Tokens = nlu.Tokens
	result.Entities = make([]*skillsAPI.Entity, len(nlu.Entities))
	for i, entity := range nlu.Entities {
		value, err := newValue(entity.Value)
		if err != nil {
			return nil, err
		}
		result.Entities[i] = &skillsAPI.Entity{
			Start:      entity.Tokens.Start,
			End:        entity.Tokens.End,
			Value:      entity.Value,
			ProtoValue: value,

			// All vins entities are prefixed with YANDEX
			Type: strings.TrimPrefix(entity.Type, "YANDEX."),
		}

	}
	return result, nil
}

func FromWebhookRequest(skillID string, request *Request) (*skillsAPI.SkillRequest, error) {
	if request == nil {
		return nil, nil
	}
	nlu, err := nluFromRequest(request.Request.Nlu)
	if err != nil {
		return nil, err
	}
	return &skillsAPI.SkillRequest{
		Meta: &skillsAPI.Meta{
			Locale:   request.Meta.Locale,
			Timezone: request.Meta.Timezone,
			ClientId: request.Meta.ClientID,
			Interfaces: &skillsAPI.Interfaces{
				Screen: request.Meta.Interfaces.Screen != nil,
			},
		},
		Request: &skillsAPI.RequestBody{
			Command:           request.Request.Command,
			OriginalUtterance: request.Request.OriginalUtterance,
			Type:              request.Request.Type,
			Payload:           request.Request.Payload,
			Nlu:               nlu,
		},
		Session: &skillsAPI.Session{
			New:       request.Session.New,
			MessageId: request.Session.MessageID,
			SessionId: request.Session.SessionID,
			UserId:    request.Session.UserID,
			SkillId:   skillID,
		},
	}, nil
}

type ResponseBody struct {
	Text       string   `json:"text"`
	Tts        string   `json:"tts"`
	Buttons    []Button `json:"buttons,omitempty"`
	Card       *Card    `json:"card,omitempty"`
	EndSession bool     `json:"end_session"`
}

type Response struct {
	Response     ResponseBody    `json:"response"`
	Session      Session         `json:"session"`
	Version      string          `json:"version"`
	SessionState json.RawMessage `json:"session_state,omitempty"`
}

func ToWebhookResponse(skillID string, protoResponse *skillsAPI.SkillResponse) (response *Response, err error) {
	if protoResponse == nil {
		return response, xerrors.New("empty proto response")
	}
	response = &Response{
		Version: "1.0",
	}
	if responseBody := protoResponse.GetResponse(); responseBody != nil {
		response.Response = ResponseBody{
			Text:       responseBody.Text,
			Tts:        responseBody.Tts,
			EndSession: responseBody.EndSession,
		}
		if responseBody.Card != nil {
			response.Response.Card, err = cardFromProto(responseBody.Card)
			if err != nil {
				return response, err
			}
		}
		if responseBody.Buttons != nil {
			response.Response.Buttons, err = buttonsFromProto(responseBody.Buttons)
			if err != nil {
				return response, err
			}
		}
	}
	if protoResponse.State != nil {
		response.SessionState = protoResponse.State.Storage
	}
	if session := protoResponse.GetSession(); session != nil {
		response.Session = Session{
			New:       session.New,
			MessageID: session.MessageId,
			SessionID: session.SessionId,
			UserID:    session.UserId,
			SkillID:   skillID,
		}
	}
	return response, nil
}
