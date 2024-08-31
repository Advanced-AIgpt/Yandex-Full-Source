package notificator

import (
	"context"
	"net/http"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

type setraceChildCaller struct{}

func (s *setraceChildCaller) CallDescription(ctx context.Context) string {
	switch ctx.Value(signalKey) {
	case sendTypedSemanticFramePushSignal:
		return "Notificator sendTypedSemanticFramePush call"
	case getDevicesSignal:
		return "Notificator getDevices call"
	default:
		return "Notificator unknown call"
	}
}

func (s *setraceChildCaller) IsCallSuccess(response *http.Response, err error) bool {
	return err == nil && response.StatusCode == http.StatusOK
}

func NewSetraceRoundTripper(logger log.Logger, transport http.RoundTripper) http.RoundTripper {
	return setrace.NewRoundTripper(transport, logger, &setraceChildCaller{})
}
