package testing

import (
	"log"
	"math/rand"

	"go.uber.org/zap"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

type TestContext struct {
	isNewSession bool
	matchesMap   map[string][]sdk.Hypothesis
}

func CreateTestContext(isNewSession bool, matchesMap map[string][]sdk.Hypothesis) *TestContext {
	return &TestContext{
		isNewSession: isNewSession,
		matchesMap:   matchesMap,
	}
}

func (context *TestContext) NextMessage() {
	context.isNewSession = false
}

func (context *TestContext) IsNewSession() bool {
	return context.isNewSession
}

func (context *TestContext) GetState() interface{} {
	return &struct{}{}
}

func (context *TestContext) Match(request *sdk.Request, _ []sdk.Pattern, extractor sdk.Extractor) ([]sdk.Hypothesis, error) {
	if extractor == nil {
		log.Fatal("Extractor should be initialized")
	}
	return context.matchesMap[request.Command], nil
}

func CreateTestLogger() sdk.Logger {
	return zap.NewNop().Sugar()
}

func DefaultMeta() *sdk.Meta {
	return &sdk.Meta{Interfaces: sdk.Interfaces{Screen: true}}
}

type SourceMock struct {
	seed int64
}

func (source *SourceMock) Int63() int64 {
	return source.seed
}

func (source *SourceMock) Seed(seed int64) {
	source.seed = seed
}

func CreateRandMock(seed int64) rand.Rand {
	return *rand.New(&SourceMock{seed: seed})
}
