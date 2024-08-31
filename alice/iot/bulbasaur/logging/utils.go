package logging

import (
	"bytes"
	"context"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"github.com/go-resty/resty/v2"
)

const bodyLengthLimit = 10000

func GetContextWithDevices(ctx context.Context, devices model.Devices) context.Context {
	var deviceIDs []string
	for _, d := range devices {
		deviceIDs = append(deviceIDs, d.ID)
	}
	return ctxlog.WithFields(ctx, log.Array("devices", deviceIDs))
}

func GetRetryableHTTPClientResponseLogHook(logger log.Logger) func(log.Logger, *http.Response) {
	return func(_ log.Logger, r *http.Response) {
		if r == nil {
			return
		}

		defer func() {
			_ = r.Body.Close()
		}()
		bodyBytes, err := ioutil.ReadAll(r.Body)
		if err != nil {
			return
		}
		r.Body = ioutil.NopCloser(bytes.NewBuffer(bodyBytes)) // hack is needed to allow further reading from response

		URL, rawResponse := Mask(r.Request.URL.String(), string(bodyBytes))

		ctxlog.Infof(r.Request.Context(), logger, "HTTP: url=%s method=%s code=%d size=%d raw_body=`%s`",
			URL, r.Request.Method, r.StatusCode, len(bodyBytes), truncateLongLogString(rawResponse))
	}
}

func GetRestyResponseLogHook(logger log.Logger) func(*resty.Client, *resty.Response) error {
	return func(c *resty.Client, r *resty.Response) error {
		if r == nil {
			return nil
		}

		URL, rawResponse := Mask(r.Request.URL, r.String())

		ctxlog.Infof(r.Request.Context(), logger, "HTTP: url=%s method=%s code=%d size=%d raw_body=`%s`",
			URL, r.Request.Method, r.StatusCode(), r.Size(), truncateLongLogString(rawResponse))
		return nil
	}
}

func GetProviderLogger(logger log.Logger, skillID string) log.Logger {
	return log.With(logger, log.String("skill_id", skillID))
}

func truncateLongLogString(line string) string {
	body := []rune(line)
	if len(body) > bodyLengthLimit {
		body = append(body[:bodyLengthLimit], []rune("...[truncated]")...)
	}
	return string(body)
}

func UserField(key string, user model.User) log.Field {
	logSafeUser := model.User{
		ID:     user.ID,
		Login:  user.Login,
		Ticket: "hidden",
	}
	return log.Any(key, logSafeUser)
}

var ProtoJSON = tools.ProtoJSONLogField // todo: remove forwarding and use tools.ProtoJSONLogField everywhere
