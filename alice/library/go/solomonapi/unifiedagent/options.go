package unifiedagent

import (
	"github.com/go-resty/resty/v2"
	"net/http"
	"time"
)

type ClientOption func(*resty.Client)

func WithTransport(transport http.RoundTripper) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetTransport(transport)
	}
}

func WithTimeout(timeout time.Duration) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetTimeout(timeout)
	}
}
