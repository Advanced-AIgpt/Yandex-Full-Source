package proxy

import (
	"a.yandex-team.ru/library/go/yandex/tvm"
	"context"
	"net/http"
)

type transportMock struct {
	transportFunc func(r *http.Request) (*http.Response, error)
}

func (t transportMock) RoundTrip(r *http.Request) (*http.Response, error) {
	return t.transportFunc(r)
}

type panicTvmClient struct {
	Panic interface{}
}

func (p panicTvmClient) GetServiceTicketForAlias(ctx context.Context, alias string) (string, error) {
	panic(p.Panic)
}

func (p panicTvmClient) GetServiceTicketForID(ctx context.Context, dstID tvm.ClientID) (string, error) {
	panic(p.Panic)
}

func (p panicTvmClient) CheckServiceTicket(ctx context.Context, ticket string) (*tvm.CheckedServiceTicket, error) {
	panic(p.Panic)
}

func (p panicTvmClient) CheckUserTicket(ctx context.Context, ticket string, opts ...tvm.CheckUserTicketOption) (*tvm.CheckedUserTicket, error) {
	panic(p.Panic)
}

func (p panicTvmClient) GetStatus(ctx context.Context) (tvm.ClientStatusInfo, error) {
	panic(p.Panic)
}
