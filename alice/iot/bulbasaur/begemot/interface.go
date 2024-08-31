package begemot

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type IClient interface {
	GetHypotheses(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error)
}

type Mock struct {
	GetHypothesesFunc func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error)
}

func (m *Mock) GetHypotheses(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
	if m.GetHypothesesFunc != nil {
		return m.GetHypothesesFunc(ctx, query, userInfo)
	}
	return 0, nil, NoHypothesesFoundError
}
