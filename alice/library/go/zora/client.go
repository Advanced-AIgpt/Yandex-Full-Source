package zora

import (
	"net/url"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Client struct {
	authPolicy authpolicy.TVMWithClientServicePolicy
}

func NewClient(tvm tvm.Client) *Client {
	return &Client{authPolicy: authpolicy.TVMWithClientServicePolicy{
		Client: tvm,
		DstID:  tvmDstID,
	}}
}

const (
	tvmDstID uint32 = 2023123 // zora tvm id

	userAgentHeaderKey      = "X-Ya-User-Agent"
	userAgentValue          = "Yandex LLC"
	destinationURLHeaderKey = "X-Ya-Dest-Url"
	endpoint                = "http://go.zora.yandex.net:1080" // only http is supported
)

func (c *Client) Execute(request *resty.Request, method, endpointURL string) (*resty.Response, error) {
	destinationURL := endpointURL
	if encodedURLParams := request.QueryParam.Encode(); encodedURLParams != "" {
		destinationURL += "?" + encodedURLParams
		request.QueryParam = url.Values{}
	}

	request.SetHeader(userAgentHeaderKey, userAgentValue)
	request.SetHeader(destinationURLHeaderKey, destinationURL)

	if err := c.authPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("failed to apply zora auth policy: %w", err)
	}
	return request.Execute(method, endpoint)
}
