package provider

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"net/url"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/amelie/pkg/logging"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	Telegram  Name = "telegram"
	Passport  Name = "passport"
	Staff     Name = "staff"
	Bulbasaur Name = "bulbasaur"
	Bass      Name = "bass"
)

func (name Name) IsExternal() bool {
	return name == Telegram
}

const (
	bodyLengthLimit = 5000
)

var (
	hdrContentTypeKey = http.CanonicalHeaderKey("Content-Type")
	jsonContentType   = "application/json"
)

type (
	Name        string
	LoggingRule interface {
		Relevant(provider Name, u url.URL) bool
	}
	RequestLoggingRule interface {
		LoggingRule
		ProcessRequest(provider Name, u url.URL, body interface{}) (outURL string, outBody interface{})
	}
	ResponseLoggingRule interface {
		LoggingRule
		ProcessResponse(provider Name, u url.URL, rawBody string) (outBody interface{})
	}
)

var (
	requestLoggingRules  []RequestLoggingRule
	responseLoggingRules []ResponseLoggingRule
)

func makeBodyError(err error) interface{} {
	return fmt.Sprintf("<error:%s>", err)
}

func AddLoggingRule(rule LoggingRule) error {
	err := fmt.Errorf("invalid logging rule: %v", rule)
	if r, ok := rule.(RequestLoggingRule); ok {
		requestLoggingRules = append(requestLoggingRules, r)
		err = nil
	}
	if r, ok := rule.(ResponseLoggingRule); ok {
		responseLoggingRules = append(responseLoggingRules, r)
		err = nil
	}
	return err
}

func LogRequest(ctx context.Context, logger log.Logger, provider Name, request *resty.Request) {
	rawURL := request.URL
	var body interface{}
	if request.Body != nil {
		if _, ok := request.Body.(io.Reader); ok {
			body = "<io.Reader>"
		} else {
			u, err := url.Parse(rawURL)
			if err != nil {
				body = makeBodyError(err)
			} else {
				processed := false
				for _, rule := range requestLoggingRules {
					if rule.Relevant(provider, *u) {
						rawURL, body = rule.ProcessRequest(provider, *u, request.Body)
						processed = true
					}
				}
				if !processed {
					if request.Header.Get(hdrContentTypeKey) == jsonContentType {
						body, err = logging.Marshal(request.Body)
						if err != nil {
							body = makeBodyError(err)
						}
					} else {
						body = fmt.Sprintf("<%T>", request.Body)
					}
				}
			}
		}
	}
	setrace.InfoLogEvent(
		ctx,
		logger,
		fmt.Sprintf("REQUEST: provider=%s url=%s method=%s", provider, rawURL, request.Method),
		log.Any("body", body),
	)
}

func min(x, y int) int {
	if x < y {
		return x
	}
	return y
}

func LogError(ctx context.Context, logger log.Logger, provider Name, errorMsg string) {
	setrace.ErrorLogEvent(
		ctx,
		logger,
		fmt.Sprintf("ERROR: provider=%s message=%s", provider, errorMsg),
	)
}

func LogResponse(ctx context.Context, logger log.Logger, provider Name, response *resty.Response) {
	rawURL := response.Request.URL
	rawBody := response.String()
	u, err := url.Parse(rawURL)
	var body interface{}
	if err != nil {
		body = makeBodyError(err)
	} else {
		processed := false
		for _, rule := range responseLoggingRules {
			if rule.Relevant(provider, *u) {
				body = rule.ProcessResponse(provider, *u, rawBody)
				processed = true
			}
		}
		if !processed && len(rawBody) > 0 {
			body = rawBody[:min(len(rawBody), bodyLengthLimit)]
		}
	}
	setrace.InfoLogEvent(
		ctx,
		logger,
		fmt.Sprintf("RESPONSE: provider=%s status=%s size=%d", provider, response.Status(), response.Size()),
		log.Any("raw_body", body),
	)
}
