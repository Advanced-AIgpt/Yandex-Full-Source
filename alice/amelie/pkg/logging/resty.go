package logging

import (
	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type restySetter = func(*resty.Client) *resty.Client

const bodyLengthLimit = 10000

func SetRestyRequestLogging(client *resty.Client) *resty.Client {
	return client.OnBeforeRequest(func(client *resty.Client, request *resty.Request) error {
		rr := request.RawRequest
		ctx := request.Context()
		// todo: add body
		ctxlog.Infof(ctx, Logger(ctx), "REQUEST: method=%s url=%s", rr.URL.String(), request.Method)
		return nil
	})
}

func SetRestyResponseLogging(client *resty.Client) *resty.Client {
	return client.OnAfterResponse(func(client *resty.Client, response *resty.Response) error {
		if response == nil {
			return nil
		}
		url := response.Request.URL
		rawResponse := response.String()
		ctx := response.Request.Context()
		ctxlog.Infof(ctx, Logger(ctx), "HTTP: url=%s method=%s code=%d size=%d raw_body=`%s`",
			url, response.Request.Method, response.StatusCode(), response.Size(), truncateLongLogString(rawResponse))
		return nil
	})
}

func truncateLongLogString(line string) string {
	body := []rune(line)
	if len(body) > bodyLengthLimit {
		body = append(body[:bodyLengthLimit], []rune("...[truncated]")...)
	}
	return string(body)
}

func SetRestyLogging(client *resty.Client) *resty.Client {
	for _, setter := range []restySetter{
		SetRestyRequestLogging,
		SetRestyResponseLogging,
	} {
		client = setter(client)
	}
	return client
}
