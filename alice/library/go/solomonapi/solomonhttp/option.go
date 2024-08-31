package solomonhttp

import (
	"github.com/go-resty/resty/v2"
	"net/http"
	"time"
)

type ClientOption func(*resty.Client)

func WithBaseURL(baseURL string) ClientOption {
	return func(r *resty.Client) {
		r.SetBaseURL(baseURL)
	}
}

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

func WithUserAgent(userAgent string) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetHeader("User-Agent", userAgent)
	}
}

type AuthPolicy func(*resty.Client)

// WithOAuthToken add oauth token auth header to all client requests
func WithOAuthToken(token string) AuthPolicy {
	return func(r *resty.Client) {
		r.SetAuthToken(token)
		r.SetAuthScheme("OAuth")
	}
}
