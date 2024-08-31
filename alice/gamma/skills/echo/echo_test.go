package main

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
)

func TestEchoSkill(t *testing.T) {
	testContext := sdkTest.CreateTestContext(true, nil)
	skill := echoSkill{}

	context := skill.CreateContext(testContext)
	logger := sdkTest.CreateTestLogger()

	request := &sdk.Request{
		OriginalUtterance: "Hi!",
	}

	response, err := skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}
	assert.Equal(t, "Ηχω", response.Text)
	assert.Equal(t, 1, *context.GetState().(*int))

	testContext.NextMessage()

	response, err = skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}

	assert.Equal(t, "1: Hi!", response.Text)
	assert.Equal(t, 2, *context.GetState().(*int))
}
