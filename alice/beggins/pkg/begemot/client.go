package begemot

import (
	"context"
	"fmt"
	"strconv"
	"strings"

	"github.com/go-resty/resty/v2"
)

type (
	Response struct {
		Rules Rules `json:"rules"`
	}

	Rules struct {
		AliceBegginsEmbedderInput *AliceBegginsEmbedderInput `json:"AliceBegginsEmbedderInput"`
		AliceBegginsEmbedder      *AliceBegginsEmbedder      `json:"AliceBegginsEmbedder"`
	}

	AliceBegginsEmbedderInput struct {
		Query string `json:"Query"`
	}

	AliceBegginsEmbedder struct {
		SentenceEmbedding []string `json:"SentenceEmbedding"`
	}

	ClientParams struct {
		Host        string
		Rules       []string
		ExtraParams []string
		DebugLevel  int
	}
)

type Client interface {
	Query(ctx context.Context, text string) (*Response, error)
}

type client struct {
	httpClient *resty.Client
	params     ClientParams
}

func (c *client) Query(ctx context.Context, text string) (*Response, error) {
	res := &Response{}
	r, err := c.httpClient.R().
		SetContext(ctx).
		SetQueryParam("text", text).
		SetQueryParam("wizclient", "megamind").
		SetQueryParam("tld", "ru").
		SetQueryParam("format", "json").
		SetQueryParam("lang", "ru").
		SetQueryParam("uil", "ru").
		SetQueryParam("dbgwzr", strconv.Itoa(c.params.DebugLevel)).
		SetQueryParam("wizextra", strings.Join(c.params.ExtraParams, ";")).
		SetQueryParam("rwr", strings.Join(c.params.Rules, ",")).
		SetQueryParam("markup", "").
		SetResult(res).
		Get(c.url())
	if err != nil {
		return nil, err
	}
	if r.IsError() {
		return nil, fmt.Errorf("error status: %s", r.Status())
	}
	return res, nil
}

func (c *client) url() string {
	return fmt.Sprintf("http://%s/wizard", c.params.Host)
}

func NewClient(httpClient *resty.Client, params ClientParams) Client {
	return &client{
		httpClient: httpClient,
		params:     params,
	}
}
