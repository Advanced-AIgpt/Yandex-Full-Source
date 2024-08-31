package sdk

import (
	"testing"

	structpb "github.com/golang/protobuf/ptypes/struct"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/library/go/test/assertpb"
)

func TestButtonToProto(t *testing.T) {
	button := Button{
		Title: "123",
		Payload: map[string]interface{}{
			"test": "bar",
		},
	}

	protoButton, err := button.toProto()
	if err != nil {
		t.Fatalf("Error in button.toProto(): %v", err)
	}
	expectedButton := &api.Button{Title: "123", Payload: []byte(`{"test":"bar"}`)}
	assertpb.Equal(t, expectedButton, protoButton, "Buttons should be equal")
}

func TestAddButtons(t *testing.T) {
	r := Response{}

	r.AddButtons(
		Button{
			Title: "123",
			Payload: map[string]interface{}{
				"test": "bar",
			}},
		Button{
			Title: "123",
			Payload: map[string]interface{}{
				"another test": 123,
			}},
	)

	protoResponseBody, err := r.toProto()
	if err != nil {
		t.Fatalf("Error in response.toProto(): %v", err)
	}
	expectedResponse := &api.ResponseBody{
		Buttons: []*api.Button{
			{Title: "123", Payload: []byte(`{"test":"bar"}`)},
			{Title: "123", Payload: []byte(`{"another test":123}`)},
		},
	}
	assertpb.Equal(t, expectedResponse, protoResponseBody, "Responses should be equal")
}

func TestAddCardItems(t *testing.T) {
	card := Card{}

	card.AddCardItems(
		CardItem{
			ImageID: "id",
			Title:   "title",
		},
	)
	expectedCard := Card{
		Items: []CardItem{{"id", "title", "", nil}},
	}

	assert.Equal(t, expectedCard, card, "Response tts should be equal")
}

func TestAddCard(t *testing.T) {
	response := Response{}

	card := Card{}
	card.AddCardItems(
		CardItem{
			ImageID:     "id",
			Title:       "title",
			Description: "",
			Button:      nil,
		},
	)
	response.Card = &card

	expectedCard := Card{Items: []CardItem{{"id", "title", "", nil}}}
	expectedResponse := Response{Card: &expectedCard}

	assert.Equal(t, expectedResponse, response, "Response tts should be equal")
}

func TestCardHeaderToProto(t *testing.T) {
	header := CardHeader{Text: "title"}

	protoHeader := header.toProto()
	expectedHeader := &api.CardHeader{Text: "title"}
	assertpb.Equal(t, expectedHeader, protoHeader, "Headers should be equal")
}

func TestCardFooterToProto(t *testing.T) {
	footer := CardFooter{Text: "title", Button: &CardButton{Text: "title", Payload: nil}}

	protoFooter, err := footer.toProto()
	if err != nil {
		t.Fatal("No errors should be raised")
	}
	expectedFooter := &api.CardFooter{Text: "title", Button: &api.CardButton{Text: "title", Payload: nil}}
	assertpb.Equal(t, expectedFooter, protoFooter, "Footer texts should be equal")
}

func TestCardItemToProto(t *testing.T) {
	cardItem := CardItem{Title: "123", Button: &CardButton{Text: "foo"}}
	protoItem, err := cardItem.toProto()
	if err != nil {
		t.Fatal("No errors should be raised")
	}
	expectedItem := &api.CardItem{Title: "123", Button: &api.CardButton{Text: "foo", Payload: nil}}
	assertpb.Equal(t, expectedItem, protoItem, "Card titles should be equal")
}

func TestCardToProto(t *testing.T) {
	card := Card{Type: "BigImage", Items: []CardItem{{Title: "someTitle", Button: &CardButton{Text: "foo"}}}}

	protoCard, err := card.toProto()
	if err != nil {
		t.Fatal("No errors should be raised")
	}
	expectedCard := &api.Card{Type: "BigImage", Items: []*api.CardItem{{Title: "someTitle", Button: &api.CardButton{Text: "foo"}}}}
	for i := range protoCard.Items {
		assertpb.Equal(t, expectedCard.Items[i].Button, protoCard.Items[i].Button, "Card items be equal")
	}
	assertpb.Equal(t, protoCard, expectedCard, "Cards should be equal")
}

type Composite struct {
	Simple Simple         `json:"simple"`
	List   []int          `json:"list"`
	Map    map[string]int `json:"map"`
	Number int            `json:"number"`
}

type Simple struct {
	Number int    `json:"number"`
	String string `json:"string"`
}

func TestRequestFromProto(t *testing.T) {
	protoRequest := api.SkillRequest{
		Request: &api.RequestBody{
			Command:           "Test",
			OriginalUtterance: "Test",
			Type:              "SimpleUtterance",
			Nlu: &api.Nlu{
				Entities: []*api.Entity{
					{Start: 0, End: 1, ProtoValue: &structpb.Value{
						Kind: &structpb.Value_NumberValue{
							NumberValue: 123,
						},
					},
						Type: "number"},
				},
				Tokens: []string{"Test"},
			},
		},
		Session: &api.Session{
			New:       false,
			MessageId: 1,
			SessionId: "d34df00d",
			UserId:    "133780085",
			SkillId:   "7357",
		},
		State: &api.State{
			Storage: []byte(`{"simple":{"number":42,"string":"kaboom"},"list":[1,3,3,7],"map":{"cool":-1,"map":2019},"number":17}`),
		},
		Meta: &api.Meta{
			Locale:   "ru-Ru",
			Timezone: "UTC",
			ClientId: "testing device 0.1",
			Interfaces: &api.Interfaces{
				Screen: true,
			},
		},
	}

	request, err := requestFromProto(protoRequest.Request)
	require.NoError(t, err)
	assert.Equal(t, &Request{
		Command:           "Test",
		OriginalUtterance: "Test",
		Type:              "SimpleUtterance",
		Nlu: &Nlu{
			Tokens: []string{"Test"},
			Entities: []Entity{
				{Start: 0, End: 1, Value: &structpb.Value{Kind: &structpb.Value_NumberValue{NumberValue: 123}}, Type: "number"},
			},
		},
	}, request)

	expectedState := &Composite{
		Simple: Simple{Number: 42, String: "kaboom"},
		List:   []int{1, 3, 3, 7},
		Map:    map[string]int{"cool": -1, "map": 2019},
		Number: 17,
	}
	actualState := &Composite{}
	err = stateFromProto(actualState, protoRequest.State)
	require.NoError(t, err)
	assert.Equal(t, expectedState, actualState)

	meta := metaFromProto(protoRequest.Meta)
	assert.Equal(t, &Meta{
		Locale:   "ru-Ru",
		Timezone: "UTC",
		ClientID: "testing device 0.1",
		Interfaces: Interfaces{
			Screen: true,
		},
	}, meta)
}
