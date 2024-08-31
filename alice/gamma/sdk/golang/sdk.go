package sdk

import (
	"a.yandex-team.ru/alice/gamma/sdk/api"
)

type Skill interface {
	CreateContext(context Context) Context
	Setup(log Logger, options Options) error
	Handle(log Logger, context Context, request *Request, meta *Meta) (*Response, error)
	Finalize(log Logger) error
}

type BaseSkill struct{}

func (BaseSkill) CreateContext(context Context) Context {
	return context
}

func (BaseSkill) Setup(log Logger, options Options) error {
	return nil
}

func (BaseSkill) Handle(log Logger, context Context, request *Request, meta *Meta) (*Response, error) {
	return nil, nil
}

func (BaseSkill) Finalize(log Logger) error {
	return nil
}

type Response struct {
	Text       string
	Tts        string
	Buttons    []Button
	Card       *Card
	EndSession bool
}

func (response *Response) AddButtons(buttons ...Button) {
	response.Buttons = append(response.Buttons, buttons...)
}

func (response *Response) toProto() (protoResponse *api.ResponseBody, err error) {
	protoResponse = &api.ResponseBody{
		Text:       response.Text,
		Tts:        response.Tts,
		EndSession: response.EndSession,
	}
	if response.Buttons != nil {
		protoResponse.Buttons, err = buttonsToProto(response.Buttons)
		if err != nil {
			return nil, err
		}
	}
	if response.Card != nil {
		protoResponse.Card, err = response.Card.toProto()
		if err != nil {
			return nil, err
		}
	}
	return protoResponse, nil
}

type Request struct {
	Command           string
	OriginalUtterance string
	Type              string
	Payload           interface{}
	Nlu               *Nlu
}

type Meta struct {
	Locale     string
	Timezone   string
	ClientID   string
	Interfaces Interfaces
}

type Nlu struct {
	Entities []Entity
	Tokens   []string
}

type Entity struct {
	Start int64
	End   int64
	Value interface{}
	Type  string
}

type Interfaces struct {
	Screen bool
}

func (context *SkillContext) GetState() interface{} {
	return &struct{}{}
}

func (context *SkillContext) IsNewSession() bool {
	return context.session.New
}
