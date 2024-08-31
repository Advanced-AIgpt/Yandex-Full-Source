package tvm

import (
	"context"
	"net"
	"net/http"
	"time"

	"golang.org/x/xerrors"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

type ClientWithMetrics struct {
	tvmClient tvm.Client
}

func (c *ClientWithMetrics) GetServiceTicketForAlias(ctx context.Context, alias string) (string, error) {
	return c.tvmClient.GetServiceTicketForAlias(withGetServiceTicketSignal(ctx), alias)
}

func (c *ClientWithMetrics) GetServiceTicketForID(ctx context.Context, dstID tvm.ClientID) (string, error) {
	return c.tvmClient.GetServiceTicketForID(withGetServiceTicketSignal(ctx), dstID)
}

func (c *ClientWithMetrics) CheckServiceTicket(ctx context.Context, ticket string) (*tvm.CheckedServiceTicket, error) {
	return c.tvmClient.CheckServiceTicket(withCheckServiceTicketSignal(ctx), ticket)
}

func (c *ClientWithMetrics) CheckUserTicket(ctx context.Context, ticket string, opts ...tvm.CheckUserTicketOption) (*tvm.CheckedUserTicket, error) {
	return c.tvmClient.CheckUserTicket(withCheckUserTicketSignal(ctx), ticket, opts...)
}

func (c *ClientWithMetrics) GetStatus(ctx context.Context) (tvm.ClientStatusInfo, error) {
	return c.tvmClient.GetStatus(ctx)
}

func (c *ClientWithMetrics) GetRoles(ctx context.Context) (*tvm.Roles, error) {
	return c.tvmClient.GetRoles(ctx)
}

func NewDeployClientWithMetrics(registry metrics.Registry) (*ClientWithMetrics, error) {
	var tvmOptions []tvmtool.Option

	if registry != nil {
		tvmOptions = append(tvmOptions, tvmtool.WithHTTPClient(newHTTPClientWithMetrics(registry)))
	}

	tvmClient, err := tvmtool.NewDeployClient(tvmOptions...)
	if err != nil {
		return nil, err
	}

	return &ClientWithMetrics{tvmClient}, nil
}

func NewClientWithMetrics(ctx context.Context, apiURI, clientName, token string, registry metrics.Registry) (*ClientWithMetrics, error) {
	tvmOptions := []tvmtool.Option{
		tvmtool.WithSrc(clientName),
		tvmtool.WithAuthToken(token),
		tvmtool.WithCacheEnabled(true),
		tvmtool.WithBackgroundUpdate(ctx),
	}

	if registry != nil {
		tvmOptions = append(tvmOptions, tvmtool.WithHTTPClient(newHTTPClientWithMetrics(registry)))
	}

	tvmClient, err := tvmtool.NewClient(apiURI, tvmOptions...)
	if err != nil {
		return nil, err
	}

	return &ClientWithMetrics{tvmClient}, nil
}

func newHTTPClientWithMetrics(registry metrics.Registry) *http.Client {
	httpClient := &http.Client{
		Transport: &http.Transport{
			Proxy: http.ProxyFromEnvironment,
			DialContext: (&net.Dialer{
				Timeout:   100 * time.Millisecond, // from tvmtool
				KeepAlive: 60 * time.Second,       // from tvmtool
			}).DialContext,
		},
		Timeout: 500 * time.Millisecond, // from tvmtool
		CheckRedirect: func(req *http.Request, via []*http.Request) error {
			return http.ErrUseLastResponse
		},
	}

	return quasarmetrics.HTTPClientWithMetrics(httpClient, NewSignals(registry))
}

type ClientMock struct {
	Logger             log.Logger
	UserID             tvm.UID
	SrcServiceID       tvm.ClientID
	ServiceTickets     map[string]string       // dstAlias -> ticket
	ServiceTicketsByID map[tvm.ClientID]string // dstTvmID -> ticket
}

func (c ClientMock) GetServiceTicketForAlias(ctx context.Context, alias string) (string, error) {
	if ticket, ok := c.ServiceTickets[alias]; ok {
		return ticket, nil
	} else {
		return "", xerrors.Errorf("TvmMock: no service ticket for %q", alias)
	}
}

func (c ClientMock) GetServiceTicketForID(ctx context.Context, dstID tvm.ClientID) (string, error) {
	if ticket, ok := c.ServiceTicketsByID[dstID]; ok {
		return ticket, nil
	} else {
		return "", xerrors.Errorf("TvmMock: no service ticket for %d", dstID)
	}
}

func (c ClientMock) CheckServiceTicket(ctx context.Context, ticket string) (*tvm.CheckedServiceTicket, error) {
	if c.SrcServiceID == 0 {
		return nil, xerrors.Errorf("TvmMock: Invalid service ticket: %s, srcServiceId not set", ticket)
	}

	c.Logger.Infof("TvmMock: Checking service ticket %s against %v", ticket, c.SrcServiceID)
	return &tvm.CheckedServiceTicket{
		SrcID: c.SrcServiceID,
	}, nil
}

func (c ClientMock) CheckUserTicket(ctx context.Context, ticket string, opts ...tvm.CheckUserTicketOption) (*tvm.CheckedUserTicket, error) {
	if c.UserID == 0 {
		return nil, xerrors.Errorf("TvmMock: Invalid user ticket: %s, userId not set", ticket)
	}
	c.Logger.Infof("TvmMock: Checking user ticket %s: returning %d", ticket, c.UserID)
	return &tvm.CheckedUserTicket{
		DefaultUID: c.UserID,
	}, nil
}

func (c ClientMock) GetStatus(ctx context.Context) (tvm.ClientStatusInfo, error) {
	panic("implement me")
}

func (c ClientMock) GetRoles(ctx context.Context) (*tvm.Roles, error) {
	panic("implement me")
}
