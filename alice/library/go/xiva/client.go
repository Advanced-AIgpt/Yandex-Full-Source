package xiva

import (
	"context"
	"net"
	"net/http"
	"strconv"
	"time"

	"github.com/go-resty/resty/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/xiva"
	"a.yandex-team.ru/library/go/yandex/xiva/httpxiva"
)

type Client struct {
	xiva.Client
}

func NewClientWithMetrics(
	baseURL,
	subscriptionToken,
	sendToken,
	serviceName string,
	maxIdleConnections int,
	logger log.Logger,
	registry metrics.Registry,
) (*Client, error) {
	httpClient := &http.Client{
		Transport: &http.Transport{
			Proxy: http.ProxyFromEnvironment,
			DialContext: (&net.Dialer{
				Timeout:   30 * time.Second,
				KeepAlive: 30 * time.Second,
			}).DialContext,
			MaxIdleConns:          maxIdleConnections,
			IdleConnTimeout:       90 * time.Second,
			TLSHandshakeTimeout:   10 * time.Second,
			ExpectContinueTimeout: 1 * time.Second,
			MaxIdleConnsPerHost:   maxIdleConnections, // use the same value because all requests go to the same xiva host
		}, // from default createTransport in resty.New()
		Timeout: 2 * time.Second,
	}

	if registry != nil {
		httpClient = quasarmetrics.HTTPClientWithMetrics(httpClient, newSignals(registry))
	}

	restyClient := resty.NewWithClient(httpClient)
	restyClient.SetLogger(logger)
	restyClient.SetRetryCount(3)

	client, err := httpxiva.NewClientWithResty(restyClient,
		httpxiva.WithHTTPHost(baseURL),
		httpxiva.WithServiceName(serviceName),
		httpxiva.WithTokens(subscriptionToken, sendToken),
	)
	if err != nil {
		return nil, err
	}

	return &Client{client}, nil
}

func (c *Client) GetSubscriptionSign(ctx context.Context, userID string) (xiva.SubscriptionSign, error) {
	return c.Client.GetSubscriptionSign(withGetSubscriptionSignSignal(ctx), userID)
}

func (c *Client) GetWebsocketURL(ctx context.Context, userID, sessionID, clientName string, filter *xiva.Filter) (string, error) {
	return c.Client.GetWebsocketURL(withGetWebsocketURLSignal(ctx), userID, sessionID, clientName, filter)
}

func (c *Client) SendEvent(ctx context.Context, userID, eventID string, event xiva.Event) error {
	return c.Client.SendEvent(withSendPushSignal(ctx), userID, eventID, event)
}

func (c *Client) ListActiveSubscriptions(ctx context.Context, userID string) ([]xiva.Subscription, error) {
	return c.Client.ListActiveSubscriptions(withListSubscriptionsSignal(ctx), userID)
}

type MockSentEvent struct {
	UserID    uint64
	EventID   string
	EventData xiva.Event
}

type MockClient struct {
	SentEvents    chan MockSentEvent
	Subscriptions []xiva.Subscription
	Error         error
}

func NewMockClient() *MockClient {
	return &MockClient{
		SentEvents: make(chan MockSentEvent, 100000),
		Subscriptions: []xiva.Subscription{
			{
				ID:      "some_id",
				Client:  "xiva_client",
				Filter:  "",
				Session: "s322",
				TTL:     10,
				URL:     "url://callback",
			},
		},
	}
}

func (m *MockClient) GetSubscriptionSign(ctx context.Context, userID string) (xiva.SubscriptionSign, error) {
	return xiva.SubscriptionSign{Sign: "test_sign", Timestamp: "123456789"}, m.Error
}

func (m *MockClient) GetWebsocketURL(ctx context.Context, userID, sessionID, clientName string, filter *xiva.Filter) (string, error) {
	return "", m.Error
}

func (m *MockClient) SendEvent(ctx context.Context, userID, eventID string, event xiva.Event) error {
	res, err := strconv.ParseUint(userID, 10, 64)
	if err != nil {
		return err
	}
	m.SentEvents <- MockSentEvent{res, eventID, event}
	return m.Error
}

func (m *MockClient) ListActiveSubscriptions(_ context.Context, _ string) ([]xiva.Subscription, error) {
	return m.Subscriptions, m.Error
}

func (m *MockClient) AssertEvent(timeout time.Duration, assertion func(event MockSentEvent) error) error {
	select {
	case <-time.After(timeout):
		return xerrors.Errorf("no xiva event sent: timeout after %v", timeout)
	case event := <-m.SentEvents:
		return assertion(event)
	}
}

func (m *MockClient) AssertEvents(timeout time.Duration, assertion func(events map[string][]MockSentEvent)) {
	events := map[string][]MockSentEvent{}
	for {
		select {
		case <-time.After(timeout):
			assertion(events)
			return
		case event := <-m.SentEvents:
			events[event.EventID] = append(events[event.EventID], event)
		}
	}
}

func (m *MockClient) AssertNoEvents(timeout time.Duration) error {
	select {
	case <-time.After(timeout):
		return nil
	case event := <-m.SentEvents:
		return xerrors.Errorf("unexpected event: %+v", event)
	}
}

func (m *MockClient) SkipEvent(timeout time.Duration) error {
	select {
	case <-time.After(timeout):
		return xerrors.Errorf("no xiva event sent: timeout after %v", timeout)
	case <-m.SentEvents:
		return nil
	}
}
