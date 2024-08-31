package authpolicy

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type TVMFactory interface {
	NewAuthPolicy(ctx context.Context, userTicket string) (TVMPolicy, error)
}

func NewFactory(tvmClient tvm.Client, tvmDstID tvm.ClientID) TVMFactory {
	return tvmFactory{
		TVMDstID: tvmDstID,
		TVM:      tvmClient,
	}
}

type tvmFactory struct {
	TVMDstID tvm.ClientID
	TVM      tvm.Client
}

func (f tvmFactory) NewAuthPolicy(ctx context.Context, userTicket string) (TVMPolicy, error) {
	serviceTicket, err := f.TVM.GetServiceTicketForID(ctx, f.TVMDstID)
	if err != nil {
		return TVMPolicy{}, xerrors.Errorf("failed to get service tvm ticket: %v", err)
	}

	tvmPolicy := TVMPolicy{
		UserTicket:    userTicket,
		ServiceTicket: serviceTicket,
	}
	return tvmPolicy, nil
}
