package setrace

import (
	"context"
	"net/http"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
)

type ChildActivation struct {
	RequestID         string
	RequestTimestamp  int64
	ChildActivationID string
	ChildDescription  string
	IsSuccess         func(response *http.Response, err error) bool
}

type ChildCaller interface {
	CallDescription(ctx context.Context) string
	IsCallSuccess(response *http.Response, err error) bool
}

type RoundTripper struct {
	transport http.RoundTripper
	logger    log.Logger
	caller    ChildCaller
}

func (t *RoundTripper) RoundTrip(request *http.Request) (*http.Response, error) {
	ctx := request.Context()
	setraceRequestID, _ := GetMainRequestID(ctx)
	activation := ChildActivation{
		RequestID:         setraceRequestID,
		RequestTimestamp:  timestamp.Now().UnixMicro(),
		ChildActivationID: uuid.Must(uuid.NewV4()).String(),
		ChildDescription:  t.caller.CallDescription(ctx),
		IsSuccess:         t.caller.IsCallSuccess,
	}
	ChildActivationStarted(ctx, t.logger, activation.ChildActivationID, activation.ChildDescription)
	childToken := requestid.ConstructRTLogToken(activation.RequestTimestamp, activation.RequestID, activation.ChildActivationID)
	request.Header.Add(requestid.XRTLogToken, childToken)
	response, err := t.transport.RoundTrip(request)
	ChildActivationFinished(ctx, t.logger, activation.IsSuccess(response, err))
	return response, err
}

func NewRoundTripper(transport http.RoundTripper, logger log.Logger, caller ChildCaller) *RoundTripper {
	return &RoundTripper{
		transport,
		logger,
		caller,
	}
}
