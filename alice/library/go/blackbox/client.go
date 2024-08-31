package blackbox

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"runtime"
	"time"

	"github.com/go-resty/resty/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Client struct {
	Logger log.Logger

	internalClient *httpbb.Client
}

type RestyOption func(client *resty.Client) *resty.Client

func (c *Client) Init(tvmClient tvm.Client, url string, tvmDstID tvm.ClientID, registry metrics.Registry, restyOptions ...RestyOption) {
	c.Logger.Info("Initializing BlackBox Client")

	if url == "" {
		url = httpbb.ProdEnvironment.BlackboxHost
		tvmDstID = httpbb.ProdEnvironment.TvmID
	}

	httpClient := &http.Client{
		Transport: &http.Transport{
			Proxy: http.ProxyFromEnvironment,
			DialContext: (&net.Dialer{
				Timeout:   30 * time.Second,
				KeepAlive: 30 * time.Second,
			}).DialContext,
			MaxIdleConns:          100,
			IdleConnTimeout:       90 * time.Second,
			TLSHandshakeTimeout:   10 * time.Second,
			ExpectContinueTimeout: 1 * time.Second,
			MaxIdleConnsPerHost:   runtime.GOMAXPROCS(0) + 1,
		},
	} // from default createTransport in resty.New()

	if registry != nil {
		httpClient = quasarmetrics.HTTPClientWithMetrics(httpClient, newSignals(registry))
	}

	restyClient := resty.NewWithClient(httpClient)
	restyClient.SetLogger(c.Logger)

	for _, opt := range restyOptions {
		restyClient = opt(restyClient)
	}

	client, err := httpbb.NewClientWithResty(
		httpbb.Environment{
			BlackboxHost: url,
			TvmID:        tvmDstID,
		},
		restyClient,
		httpbb.WithTVM(tvmClient),
		httpbb.WithRetries(5),
		httpbb.WithTimeout(time.Second*3),
	)
	if err != nil {
		panic(fmt.Errorf("blackbox client creation failed %s", err))
	}
	c.internalClient = client

	c.Logger.Info("BlackBox Client was successfully initialized.")
}

func (c *Client) SessionID(ctx context.Context, req blackbox.SessionIDRequest) (*blackbox.SessionIDResponse, error) {
	bbResponse, err := c.internalClient.SessionID(withGetUserBySessionIDSignal(ctx), req)
	if err != nil {
		return nil, err
	}

	ctxlog.Infof(ctx, c.Logger,
		"Got data from blackbox: { uid: '%d', login: '%s'}",
		bbResponse.User.ID, bbResponse.User.Login,
	)
	return bbResponse, nil
}

func (c *Client) OAuth(ctx context.Context, req blackbox.OAuthRequest) (*blackbox.OAuthResponse, error) {
	bbResponse, err := c.internalClient.OAuth(withGetUserByOauthSignal(ctx), req)
	if err != nil {
		return nil, err
	}

	ctxlog.Infof(ctx, c.Logger,
		"Got data from blackbox: { uid: '%d', login: '%s', oauth: { scope: '%s' }}",
		bbResponse.User.UID.ID, bbResponse.User.Login, bbResponse.Scopes,
	)
	return bbResponse, nil
}

func (c *Client) MultiSessionID(ctx context.Context, req blackbox.MultiSessionIDRequest) (*blackbox.MultiSessionIDResponse, error) {
	return c.internalClient.MultiSessionID(ctx, req)
}

func (c *Client) UserInfo(ctx context.Context, req blackbox.UserInfoRequest) (*blackbox.UserInfoResponse, error) {
	return c.internalClient.UserInfo(withGetUserInfoSignal(ctx), req)
}

func (c *Client) UserTicket(ctx context.Context, req blackbox.UserTicketRequest) (*blackbox.UserTicketResponse, error) {
	return c.internalClient.UserTicket(ctx, req)
}

func (c *Client) CheckIP(ctx context.Context, req blackbox.CheckIPRequest) (*blackbox.CheckIPResponse, error) {
	return c.internalClient.CheckIP(ctx, req)
}

type ClientMock struct {
	Logger     log.Logger
	User       *userctx.User
	OAuthToken *userctx.OAuth
}

func (c *ClientMock) SessionID(ctx context.Context, req blackbox.SessionIDRequest) (*blackbox.SessionIDResponse, error) {
	if c.User == nil {
		return &blackbox.SessionIDResponse{}, xerrors.Errorf("BlackboxMock: User not set")
	}
	c.Logger.Infof("BlackboxMock: Returning stored user and ticket %v", c.User)

	return &blackbox.SessionIDResponse{
		User:       blackbox.User{ID: c.User.ID, Login: c.User.Login},
		UserTicket: c.User.Ticket,
	}, nil
}

func (c *ClientMock) OAuth(ctx context.Context, req blackbox.OAuthRequest) (*blackbox.OAuthResponse, error) {
	if c.User == nil {
		return &blackbox.OAuthResponse{}, xerrors.Errorf("BlackboxMock: User not set")
	}
	if c.OAuthToken == nil {
		return &blackbox.OAuthResponse{}, xerrors.Errorf("BlackboxMock: OAuthToken not set")
	}
	c.Logger.Infof("BlackboxMock: Returning stored user, ticket and OAuth token %v, %v ", c.User, c.OAuthToken)

	return &blackbox.OAuthResponse{
		User:       blackbox.User{ID: c.User.ID, Login: c.User.Login},
		UserTicket: c.User.Ticket,
		ClientID:   c.OAuthToken.ClientID,
		Scopes:     c.OAuthToken.Scope,
	}, nil
}

func (c *ClientMock) MultiSessionID(ctx context.Context, req blackbox.MultiSessionIDRequest) (*blackbox.MultiSessionIDResponse, error) {
	panic("Not implemented")
}

func (c *ClientMock) UserInfo(ctx context.Context, req blackbox.UserInfoRequest) (*blackbox.UserInfoResponse, error) {
	if c.User == nil {
		return &blackbox.UserInfoResponse{}, xerrors.Errorf("BlackboxMock: User not set")
	}
	return &blackbox.UserInfoResponse{Users: []blackbox.User{{ID: c.User.ID, Login: c.User.Login}}}, nil
}

func (c *ClientMock) UserTicket(ctx context.Context, req blackbox.UserTicketRequest) (*blackbox.UserTicketResponse, error) {
	panic("not implemented")
}

func (c *ClientMock) CheckIP(ctx context.Context, req blackbox.CheckIPRequest) (*blackbox.CheckIPResponse, error) {
	panic("Not implemented")
}
