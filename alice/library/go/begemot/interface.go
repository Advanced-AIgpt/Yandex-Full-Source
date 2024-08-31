package libbegemot

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	Wizard(ctx context.Context, request WizardRequest) (*WizardResponse, error)
}

type Mock struct {
	WizardFunc func(ctx context.Context, request WizardRequest) (*WizardResponse, error)
}

func (m *Mock) Wizard(ctx context.Context, request WizardRequest) (*WizardResponse, error) {
	if m.WizardFunc != nil {
		return m.WizardFunc(ctx, request)
	}
	return nil, xerrors.Errorf("wizardFunc not set")
}
