package server

import (
	"context"
	"fmt"
	"net/url"
	"strings"

	"a.yandex-team.ru/alice/amelie/internal/provider"
)

const (
	hiddenBodyPlaceholder = "<hidden>"
)

type telegramLoggingRule struct {
}

func (r *telegramLoggingRule) ProcessRequest(provider provider.Name, u url.URL, body interface{}) (outURL string, outBody interface{}) {
	outBody = body
	if u.Host == "api.telegram.org" {
		splits := strings.Split(u.Path, "/")
		if len(splits) > 0 {
			u.Path = "/xxx/" + splits[len(splits)-1]
		}
		outURL = u.String()
	}
	return
}

func (*telegramLoggingRule) Relevant(providerName provider.Name, u url.URL) bool {
	return providerName == provider.Telegram
}

type passportLoggingRule struct {
}

func (r *passportLoggingRule) ProcessResponse(provider provider.Name, u url.URL, rawBody string) (outBody interface{}) {
	return hiddenBodyPlaceholder
}

func (r *passportLoggingRule) ProcessRequest(provider provider.Name, u url.URL, body interface{}) (outURL string, outBody interface{}) {
	return u.String(), hiddenBodyPlaceholder
}

func (*passportLoggingRule) Relevant(providerName provider.Name, u url.URL) bool {
	return providerName == provider.Passport
}

func (s *Server) initProviderLoggingRules(ctx context.Context) error {
	for _, rule := range []provider.LoggingRule{
		&telegramLoggingRule{},
		&passportLoggingRule{},
	} {
		if err := provider.AddLoggingRule(rule); err != nil {
			return fmt.Errorf("init provider logging rule error: %w", err)
		}
	}
	return nil
}
