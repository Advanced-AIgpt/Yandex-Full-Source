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

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type RESTProvider struct {
	origin     model.Origin
	skillInfo  SkillInfo
	Logger     log.Logger
	client     *resty.Client
	zoraClient *zora.Client

	authPolicy authpolicy.HTTPPolicy

	skillSignals Signals
	Normalizer
}

func newRESTProvider(origin model.Origin, skillInfo SkillInfo, logger log.Logger, httpClient *http.Client, zoraClient *zora.Client, authPolicy authpolicy.HTTPPolicy, signals Signals) RESTProvider {
	providerLogger := logging.GetProviderLogger(logger, skillInfo.SkillID)
	rp := RESTProvider{
		origin:    origin,
		skillInfo: skillInfo,
		Logger:    providerLogger,

		zoraClient: zoraClient,

		authPolicy: authPolicy,

		skillSignals: signals,
		Normalizer:   Normalizer{logger: providerLogger},
	}

	client := resty.NewWithClient(httpClient)
	client = client.OnBeforeRequest(rp.onBeforeRequestLogHook).OnAfterResponse(rp.onAfterResponseLogHook)
	client = client.OnAfterResponse(logging.GetRestyResponseLogHook(providerLogger))
	client = client.SetPreRequestHook(rp.onBeforeRequestXRequestIDHeader)
	rp.client = client
	return rp
}

func (rp *RESTProvider) GetOrigin() model.Origin {
	return rp.origin
}

func (rp *RESTProvider) GetSkillInfo() SkillInfo {
	return rp.skillInfo
}

func (rp *RESTProvider) GetSkillSignals() Signals {
	return rp.skillSignals
}

func (rp *RESTProvider) Discover(ctx context.Context) (adapter.DiscoveryResult, error) {
	requestURL := tools.URLJoin(rp.skillInfo.Endpoint, "/v1.0/user/devices")

	var response adapter.DiscoveryResult
	rawResp, err := rp.simpleGetRequest(ctx, requestURL, rp.skillSignals.discovery.RequestSignals)
	if err != nil {
		return response, err
	}
	if err := binder.Bind(valid.NewValidationCtx(), rawResp, &response); err != nil {
		return response, xerrors.Errorf("failed to get provider discovery response: %w", BadResponseError{err: err})
	}
	return response, err
}

func (rp *RESTProvider) Query(ctx context.Context, statesRequest adapter.StatesRequest) (adapter.StatesResult, error) {
	var response adapter.StatesResult

	requestURL := tools.URLJoin(rp.skillInfo.Endpoint, "/v1.0/user/devices/query")
	result, err := rp.simplePostRequest(ctx, requestURL, statesRequest, rp.skillSignals.query.GetRequestSignals())
	if err != nil {
		response = rp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, err
	}
	if err := json.Unmarshal(result, &response); err != nil {
		response = rp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, xerrors.Errorf("failed to unmarshal provider states response: %q: %w", result, BadResponseError{err: err})
	}
	response = rp.normalizeStatesResult(ctx, statesRequest, response, adapter.ErrorCode(model.UnknownError))
	return response, nil
}

func (rp *RESTProvider) Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error) {
	var response adapter.ActionResult

	var totalActionsCount float64
	for _, device := range actionRequest.Payload.Devices {
		totalActionsCount += float64(len(device.Capabilities))
	}
	defer func() {
		responseErrs := response.GetErrors()
		ctxlog.Debugf(ctx, rp.logger, "provider '%s' actions stat: actions = %d, failed = %v", rp.skillInfo.SkillID, int(totalActionsCount), responseErrs)
	}()

	requestURL := tools.URLJoin(rp.skillInfo.Endpoint, "/v1.0/user/devices/action")
	result, err := rp.simplePostRequest(ctx, requestURL, actionRequest, rp.skillSignals.action.GetRequestSignals())
	if err != nil {
		switch {
		case xerrors.Is(err, &HTTPAuthorizationError{}):
			response = rp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.AccountLinkingError))
			return response, nil
		default:
			response = rp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
			return response, err
		}
	}
	if err := json.Unmarshal(result, &response); err != nil {
		response = rp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
		return response, xerrors.Errorf("failed to unmarshal provider action response: %w", BadResponseError{err: err})
	}
	response = rp.normalizeActionResult(ctx, actionRequest, response, adapter.ErrorCode(model.UnknownError))
	return response, nil
}

func (rp *RESTProvider) Unlink(ctx context.Context) error {
	requestURL := tools.URLJoin(rp.skillInfo.Endpoint, "/v1.0/user/unlink")
	_, err := rp.simplePostRequest(ctx, requestURL, nil, rp.skillSignals.unlink)
	return err
}

func (rp *RESTProvider) simpleGetRequest(ctx context.Context, url string, signals RequestSignals) ([]byte, error) {
	return rp.simpleHTTPRequest(ctx, resty.MethodGet, url, nil, signals)
}

func (rp *RESTProvider) simplePostRequest(ctx context.Context, url string, payload interface{}, signals RequestSignals) ([]byte, error) {
	return rp.simpleHTTPRequest(ctx, resty.MethodPost, url, payload, signals)
}

func (rp *RESTProvider) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}, signals RequestSignals) ([]byte, error) {
	request := rp.client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestid.GetRequestID(ctx)).
		SetHeader(headers.UserAgentKey, UserAgentValue).
		SetBody(payload)

	if payload != nil {
		request.SetHeader(headers.ContentTypeKey, headers.TypeApplicationJSON.String())
	}

	if err := rp.authPolicy.Apply(request); err != nil {
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

	if experiments.EnableGoZora.IsEnabled(ctx) && !rp.skillInfo.IsIntranet() {
		response, err = rp.zoraClient.Execute(request, method, url)
	} else {
		response, err = request.Execute(method, url)
	}

	if err != nil {
		if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
			signals.errorTimeout.Inc()
		} else {
			signals.errorOther.Inc()
		}
		return nil, xerrors.Errorf("HTTP request to Provider <%s> has failed: %w", rp.skillInfo.SkillID, err)
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
		return nil, xerrors.Errorf("request to Provider <%s> has failed. Reason: [%d] %w", rp.skillInfo.SkillID, response.StatusCode(), &HTTPAuthorizationError{})
	case response.StatusCode() != 200:
		return nil, fmt.Errorf("request to Provider <%s> has failed. Reason: [%d]", rp.skillInfo.SkillID, response.StatusCode())
	}

	responseBody := response.Body()
	return responseBody, nil
}

func (rp *RESTProvider) onBeforeRequestLogHook(_ *resty.Client, r *resty.Request) error {
	logger := recorder.GetLoggerWithDebugInfoBySkillID(rp.logger, r.Context(), rp.skillInfo.SkillID)

	method := r.Method
	url := r.URL
	requestID := r.Header.Get(requestid.XRequestID)
	bodyBytes, err := json.Marshal(r.Body)
	if err != nil {
		return err
	}
	ctxlog.Infof(r.Context(), logger, "Sending request to provider:\n%s %s\nrequest id: %s\n%s", method, url, requestID, bodyBytes)
	return nil
}

// 2020-04-09 LG asked us to be backward compatible friendly, eta to fix 1mnth
func (rp *RESTProvider) onBeforeRequestXRequestIDHeader(_ *resty.Client, r *http.Request) error {
	hdrMap := make(map[string][]string)
	for k, v := range r.Header {
		hdrMap[k] = v
		if k == requestid.XRequestID && (rp.skillInfo.SkillID == model.LGSkill || rp.skillInfo.SkillID == model.PerenioSkill) {
			hdrMap["x-request-id"] = v
			delete(hdrMap, requestid.XRequestID)
		}
	}
	r.Header = hdrMap
	return nil
}

func (rp *RESTProvider) onAfterResponseLogHook(_ *resty.Client, r *resty.Response) error {
	logger := recorder.GetLoggerWithDebugInfoBySkillID(rp.logger, r.Request.Context(), rp.skillInfo.SkillID)

	statusCode := r.StatusCode()
	_, body := logging.Mask(r.Request.URL, string(r.Body()))
	ctxlog.Infof(r.Request.Context(), logger, "Got response from provider %s:\n%d\n%s", rp.skillInfo.SkillID, statusCode, body)
	return nil
}
