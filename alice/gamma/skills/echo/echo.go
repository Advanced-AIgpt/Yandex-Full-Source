package main

import (
	"fmt"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

type echoSkill struct {
	sdk.BaseSkill
}

type MyContext struct {
	sdk.Context
	Value int
}

func (context *MyContext) GetState() interface{} {
	return &context.Value
}

func (skill *echoSkill) CreateContext(context sdk.Context) sdk.Context {
	return &MyContext{context, 0}
}

func (skill *echoSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	ctx, ok := context.(*MyContext)
	if !ok {
		return nil, xerrors.New("invalid stored context")
	}

	var response sdk.Response

	if ctx.IsNewSession() {
		response.Text = "Ηχω"
		ctx.Value = 0
	} else {
		response.Text = fmt.Sprintf("%d: %s", ctx.Value, request.OriginalUtterance)
	}
	ctx.Value++
	return &response, nil
}

func main() {
	sdk.StartSkill(&echoSkill{}, sdk.DefaultOptions{})
}
