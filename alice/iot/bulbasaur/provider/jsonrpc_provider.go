package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/valid"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type JSONRPCProvider struct {
	origin    model.Origin
	skillInfo SkillInfo
	Logger    log.Logger

	client     *resty.Client
	zoraClient *zora.Client

	rpcAuthPolicy  RPCAuthPolicy
	httpAuthPolicy authpolicy.HTTPPolicy

	skillSignals Signals
	Normalizer
}

func newJSONRPCProvider(
	origin model.Origin,
	skillInfo SkillInfo,
	logger log.Logger,
	httpClient *http.Client,
	zoraClient *zora.Client,
	httpAuthPolicy authpolicy.HTTPPolicy,
	rpcAuthPolicy RPCAuthPolicy,
	signals Signals,
) JSONRPCProvider {
	providerLogger := logging.GetProviderLogger(logger, skillInfo.SkillID)
	jp := JSONRPCProvider{
		origin:    origin,
		skillInfo: skillInfo,
		Logger:    providerLogger,

		zoraClient: zoraClient,

		rpcAuthPolicy:  rpcAuthPolicy,
		httpAuthPolicy: httpAuthPolicy,

		skillSignals: signals,
		Normalizer:   Normalizer{logger: providerLogger},
	}
	client := resty.NewWithClient(httpClient)
	client = client.OnBeforeRequest(jp.onBeforeRequestLogHook).OnAfterResponse(jp.onAfterResponseLogHook)
	client = client.OnAfterResponse(logging.GetRestyResponseLogHook(providerLogger))
	jp.client = client
	return jp
}

func (jp *JSONRPCProvider) GetOrigin() model.Origin {
	return jp.origin
}

func (jp *JSONRPCProvider) GetSkillInfo() SkillInfo {
	return jp.skillInfo
}

func (jp *JSONRPCProvider) GetSkillSignals() Signals {
	return jp.skillSignals
}

func (jp *JSONRPCProvider) Discover(ctx context.Context) (adapter.DiscoveryResult, error) {
	var response adapter.DiscoveryResult
	request := adapter.JSONRPCRequest{
		Headers:     adapter.JSONRPCHeaders{RequestID: requestid.GetRequestID(ctx)},
		RequestType: adapter.Discovery,
	}

	rawResp, err := jp.simpleHTTPRequest(ctx, request, jp.skillSignals.discovery.RequestSignals)
	if err != nil {
		return response, err
	}
	if err := binder.Bind(valid.NewValidationCtx(), rawResp, &response); err != nil {
		ctxlog.Warn(ctx, jp.Logger, "discovery bind failed", log.Any("body", string(rawResp)))
		return response, xerrors.Errorf("failed to get provider discovery response: %w", BadResponseError{err: err})
	}
	return response, err
}

func (jp *JSONRPCProvider) Query(ctx context.Context, statesRequest adapter.StatesRequest) (adapter.StatesResult, error) {
	var response adapter.StatesResult

	request := adapter.JSONRPCRequest{
		Headers:     adapter.JSONRPCHeaders{RequestID: requestid.GetRequestID(ctx)},
		RequestType: adapter.Query,
		Payload:     statesRequest,
	}

	result, err := jp.simpleHTTPRequest(ctx, request, jp.skillSignals.query.GetRequestSignals())
	if err != nil {
		response = jp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, err
	}

	if err := json.Unmarshal(result, &response); err != nil {
		response = jp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, xerrors.Errorf("failed to unmarshal provider states response: %q: %w", result, BadResponseError{err: err})
	}
	response = jp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
	return response, nil
}

func (jp *JSONRPCProvider) Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error) {
	var response adapter.ActionResult

	request := adapter.JSONRPCRequest{
		Headers:     adapter.JSONRPCHeaders{RequestID: requestid.GetRequestID(ctx)},
		RequestType: adapter.Action,
		Payload:     actionRequest.Payload,
	}

	result, err := jp.simpleHTTPRequest(ctx, request, jp.skillSignals.action.GetRequestSignals())
	if err != nil {
		switch {
		case xerrors.Is(err, &HTTPAuthorizationError{}):
			response = jp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.AccountLinkingError))
			return response, nil
		default:
			response = jp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
			return response, err
		}
	}
	if err := json.Unmarshal(result, &response); err != nil {
		response = jp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, xerrors.Errorf("failed to unmarshal provider action response: %w", BadResponseError{err: err})
	}
	response = jp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
	return response, nil
}

func (jp *JSONRPCProvider) Unlink(ctx context.Context) error {
	request := adapter.JSONRPCRequest{
		Headers:     adapter.JSONRPCHeaders{RequestID: requestid.GetRequestID(ctx)},
		RequestType: adapter.Unlink,
	}

	_, err := jp.simpleHTTPRequest(ctx, request, jp.skillSignals.unlink)
	return err
}

func (jp *JSONRPCProvider) simpleHTTPRequest(ctx context.Context, rpcRequest adapter.JSONRPCRequest, signals RequestSignals) ([]byte, error) {
	jp.rpcAuthPolicy.Apply(&rpcRequest)

	request := jp.client.R().
		SetContext(ctx).
		SetHeader(headers.UserAgentKey, UserAgentValue).
		SetHeader(headers.ContentTypeKey, headers.TypeApplicationJSON.String()).
		SetBody(rpcRequest)

	if err := jp.httpAuthPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("failed to apply http auth policy: %w", err)
	}

	start := time.Now()
	defer func() {
		signals.total.Inc()
		signals.duration.RecordDuration(time.Since(start))
	}()

	var (
		response *resty.Response
		err      error
	)
	if experiments.EnableGoZora.IsEnabled(ctx) {
		response, err = jp.zoraClient.Execute(request, resty.MethodPost, jp.skillInfo.Endpoint)
	} else {
		response, err = request.Execute(resty.MethodPost, jp.skillInfo.Endpoint)
	}
	if err != nil {
		if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
			signals.errorTimeout.Inc()
		} else {
			signals.errorOther.Inc()
		}
		return nil, xerrors.Errorf("HTTP request to Provider Adapter failed: %w", err)
	}

	switch statusCode := response.StatusCode(); {
	case statusCode == 200:
		// do nothing, will record ok later
	case statusCode >= 300 && statusCode < 400:
		signals.errorHTTP3xx.Inc()
	case statusCode >= 400 && statusCode < 500:
		signals.errorHTTP4xx.Inc()
	case statusCode >= 500 && statusCode < 600:
		signals.errorHTTP5xx.Inc()
	default:
		signals.errorOther.Inc()
	}
	switch {
	case response.StatusCode() == 401 || response.StatusCode() == 403:
		return nil, xerrors.Errorf("request to provider failed: [%d] %w", response.StatusCode(), &HTTPAuthorizationError{})
	case response.StatusCode() != 200:
		return nil, fmt.Errorf("request to provider failed: [%d]", response.StatusCode())
	}

	responseBody := response.Body()
	return responseBody, nil
}

func (jp *JSONRPCProvider) onBeforeRequestLogHook(_ *resty.Client, r *resty.Request) error {
	logger := recorder.GetLoggerWithDebugInfoBySkillID(jp.Logger, r.Context(), jp.skillInfo.SkillID)
	method := r.Method
	url := r.URL
	rpcRequest, isRPCRequest := r.Body.(adapter.JSONRPCRequest)
	if !isRPCRequest {
		return xerrors.New("request body is not a json rpc request")
	}
	rpcRequest.Headers.Authorization = "##########"
	requestID := rpcRequest.Headers.RequestID
	bytes, err := json.Marshal(rpcRequest)
	if err != nil {
		return err
	}
	ctxlog.Infof(r.Context(), logger, "Sending request to provider:\n%s %s\nrequest id: %s\n%s", method, url, requestID, bytes)
	return nil
}

func (jp *JSONRPCProvider) onAfterResponseLogHook(_ *resty.Client, r *resty.Response) error {
	logger := recorder.GetLoggerWithDebugInfoBySkillID(jp.Logger, r.Request.Context(), jp.skillInfo.SkillID)
	statusCode := r.StatusCode()
	_, body := logging.Mask(r.Request.URL, string(r.Body()))
	ctxlog.Infof(r.Request.Context(), logger, "Got response from provider:\n%d\n%s", statusCode, body)
	return nil
}
