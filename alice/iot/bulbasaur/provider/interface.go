package provider

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IProvider interface {
	GetOrigin() model.Origin
	GetSkillInfo() SkillInfo
	GetSkillSignals() Signals
	Discover(ctx context.Context) (adapter.DiscoveryResult, error)
	Query(ctx context.Context, statesRequest adapter.StatesRequest) (adapter.StatesResult, error)
	Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error)
	Unlink(ctx context.Context) error
}

type IProviderFactory interface {
	NewProviderClient(ctx context.Context, origin model.Origin, skillID string) (IProvider, error)
	SkillInfo(ctx context.Context, skillID string, userTicket string) (SkillInfo, error)
	GetSignalsRegistry() *SignalsRegistry
}
