package provider

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/requestid"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	coremetrics "a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/mock"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FactoryMock struct {
	logger          log.Logger
	providers       map[uint64]map[string]*Mock // userID -> [skillID -> [provider]]
	signalsRegistry *SignalsRegistry
}

func NewFactoryMock(logger log.Logger) *FactoryMock {
	signalsRegistry := &SignalsRegistry{
		providerRegistry: mock.NewRegistry(mock.NewRegistryOpts()),
		policy:           metrics.DefaultExponentialBucketsPolicy,
	}
	return &FactoryMock{providers: make(map[uint64]map[string]*Mock), logger: logger, signalsRegistry: signalsRegistry}
}

func (f *FactoryMock) NewProviderClient(_ context.Context, origin model.Origin, skillID string) (IProvider, error) {
	f.logger.Infof("Creating provider client for %d with skillId %s", origin.User.ID, skillID)
	if userProviders, ok := f.providers[origin.User.ID]; ok {
		if providerClient, ok := userProviders[skillID]; ok && providerClient != nil {
			if providerClient.err != nil {
				return nil, providerClient.err
			}
			return providerClient, nil
		}
	}
	return nil, nil
}

func (f *FactoryMock) SkillInfo(_ context.Context, skillID string, _ string) (SkillInfo, error) {
	return SkillInfo{
		HumanReadableName: "Тестовый провайдер",
		ApplicationName:   skillID,
		SkillID:           skillID,
		Trusted:           false,
	}, nil
}

func (f *FactoryMock) GetSignalsRegistry() *SignalsRegistry {
	return f.signalsRegistry
}

func (f *FactoryMock) NewProvider(user *model.User, skillID string, trusted bool) *Mock {
	userProviders, ok := f.providers[user.ID]
	if !ok {
		userProviders = make(map[string]*Mock)
	}
	skillInfo, _ := f.SkillInfo(context.Background(), skillID, "")
	skillInfo.Trusted = trusted
	provider := &Mock{
		origin:             model.Origin{}, // pass it here if needed
		skillInfo:          skillInfo,
		skillSignals:       NewSkillSignalsMock(),
		logger:             f.logger,
		normalizer:         Normalizer{logger: f.logger},
		discoveryRequests:  make(map[string]int),
		discoveryResponses: make(map[string]adapter.DiscoveryResult),

		queryRequests:  make(map[string][]adapter.StatesRequest),
		queryWaitTimes: make(map[string]time.Duration),
		queryResponses: make(map[string]adapter.StatesResult),
		queryErrors:    make(map[string]error),

		actionRequests:  make(map[string][]adapter.ActionRequest),
		actionResponses: make(map[string]adapter.ActionResult),
		actionErrors:    make(map[string]error),

		unlinkRequests:  make(map[string]int),
		unlinkResponses: make(map[string]struct{}),
	}
	userProviders[skillID] = provider
	f.providers[user.ID] = userProviders
	return provider
}

func (f *FactoryMock) GetProvider(userID uint64, skillID string) *Mock {
	return f.providers[userID][skillID]
}

type Mock struct {
	origin       model.Origin
	skillInfo    SkillInfo
	skillSignals Signals
	logger       log.Logger
	normalizer   Normalizer

	successDiscoveryDevices int
	ignoredDiscoveryDevices int
	discoveryRequests       map[string]int                     // requestID -> count
	discoveryResponses      map[string]adapter.DiscoveryResult // requestID -> discoveryResult

	queryRequests  map[string][]adapter.StatesRequest // requestID -> received query requests
	queryWaitTimes map[string]time.Duration           // requestID -> wait time for device
	queryResponses map[string]adapter.StatesResult    // requestID -> statesResult
	queryErrors    map[string]error                   // requestID -> query error

	actionRequests  map[string][]adapter.ActionRequest // requestID -> received action requests
	actionResponses map[string]adapter.ActionResult    // requestID -> actionResult
	actionErrors    map[string]error                   // requestID -> action error

	unlinkRequests  map[string]int // requestID -> count
	unlinkResponses map[string]struct{}

	err error
}

func (p *Mock) WithClientError(err error) *Mock {
	p.err = err
	return p
}

func (p *Mock) GetOrigin() model.Origin {
	return p.origin
}
func (p *Mock) GetSkillInfo() SkillInfo {
	return p.skillInfo
}

func (p *Mock) GetSkillSignals() Signals {
	return p.skillSignals
}

func (p *Mock) IncDiscoverySuccess() {
	p.successDiscoveryDevices++
}

func (p *Mock) IncDiscoveryValidationError() {
	p.ignoredDiscoveryDevices++
}

func (p *Mock) GetDiscoveryValidationErrorCount() int {
	return p.ignoredDiscoveryDevices
}

func (p *Mock) WithDiscoveryResponses(responses map[string]adapter.DiscoveryResult) *Mock {
	p.discoveryResponses = responses
	return p
}

func (p *Mock) Discover(ctx context.Context) (adapter.DiscoveryResult, error) {
	requestID := requestid.GetRequestID(ctx)

	p.discoveryRequests[requestID]++
	p.logger.Infof("Performing discovery, skillId: %s, requestId: %s", p.skillInfo.SkillID, requestID)

	if _, ok := p.discoveryResponses[requestID]; !ok {
		return adapter.DiscoveryResult{}, xerrors.New("can't find discovery response")
	}

	return p.discoveryResponses[requestID], nil
}

func (p *Mock) DiscoveryCalls(requestID string) int {
	return p.discoveryRequests[requestID]
}

func (p *Mock) WithQueryWaitTime(queryWaitTimes map[string]time.Duration) *Mock {
	p.queryWaitTimes = queryWaitTimes
	return p
}

func (p *Mock) WithQueryResponses(responses map[string]adapter.StatesResult) *Mock {
	p.queryResponses = responses
	return p
}

func (p *Mock) WithQueryErrors(errors map[string]error) *Mock {
	for requestID := range errors {
		p.queryResponses[requestID] = adapter.StatesResult{}
	}
	p.queryErrors = errors
	return p
}

func (p *Mock) Query(ctx context.Context, statesRequest adapter.StatesRequest) (adapter.StatesResult, error) {
	requestID := requestid.GetRequestID(ctx)

	p.queryRequests[requestID] = append(p.queryRequests[requestID], statesRequest)
	p.logger.Infof("Performing query, skillId: %s, requestId: %s", p.skillInfo.SkillID, requestID)

	if waitTime, shouldWait := p.queryWaitTimes[requestID]; shouldWait {
		<-time.After(waitTime)
	}
	if _, ok := p.queryResponses[requestID]; !ok {
		return adapter.StatesResult{}, xerrors.New("can't find query response")
	}

	return p.normalizer.normalizeStatesResult(ctx, statesRequest, p.queryResponses[requestID], adapter.ErrorCode(model.UnknownError)), p.queryErrors[requestID]
}

func (p *Mock) QueryCalls(requestID string) []adapter.StatesRequest {
	return p.queryRequests[requestID]
}

func (p *Mock) WithActionResponses(responses map[string]adapter.ActionResult) *Mock {
	p.actionResponses = responses
	return p
}

func (p *Mock) WithActionErrors(errors map[string]error) *Mock {
	for requestID := range errors {
		p.actionResponses[requestID] = adapter.ActionResult{}
	}
	p.actionErrors = errors
	return p
}

func (p *Mock) ActionCalls(requestID string) []adapter.ActionRequest {
	return p.actionRequests[requestID]
}

func (p *Mock) Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error) {
	requestID := requestid.GetRequestID(ctx)

	p.actionRequests[requestID] = append(p.actionRequests[requestID], actionRequest)
	p.logger.Infof("performing action, skill_id: %s, request_id: %s", p.skillInfo.SkillID, requestID)

	if _, ok := p.actionResponses[requestID]; !ok {
		return adapter.ActionResult{}, xerrors.New("can't find action response")
	}

	return p.normalizer.normalizeActionResult(ctx, actionRequest, p.actionResponses[requestID], adapter.InternalError), nil
}

func (p *Mock) WithUnlinkResponses(requestIDs ...string) {
	for _, requestID := range requestIDs {
		p.unlinkResponses[requestID] = struct{}{}
	}
}

func (p *Mock) Unlink(ctx context.Context) error {
	requestID := requestid.GetRequestID(ctx)

	p.unlinkRequests[requestID]++
	p.logger.Infof("Performing unlink, skillId: %s, requestId: %s", p.skillInfo.SkillID, requestID)

	if _, ok := p.unlinkResponses[requestID]; !ok {
		return xerrors.New("can't find unlink response")
	}
	return nil
}

func (p *Mock) UnlinkCalls(requestID string) int {
	return p.unlinkRequests[requestID]
}

func NewJSONRPCProviderMock(endpoint, applicationName, humanReadableName, skillID, token, ticket string) *JSONRPCProvider {
	skillInfo := SkillInfo{
		ApplicationName:   applicationName,
		HumanReadableName: humanReadableName,
		SkillID:           skillID,
		Endpoint:          endpoint,
		Trusted:           false,
	}
	logger := testing.NopLogger()
	signals := Signals{
		discovery:  NewDiscoverySignalsMock(),
		query:      NewQuerySignalsMock(),
		action:     NewActionSignalsMock(),
		unlink:     NewRequestSignalsMock(),
		hubRemotes: NewHubRemotesSignalsMock(),
		delete:     NewDeleteSignalsMock(),
		rename:     NewRequestSignalsMock(),
	}
	origin := model.Origin{}

	p := newJSONRPCProvider(origin, skillInfo, logger, http.DefaultClient, zora.NewClient(quasartvm.ClientMock{}), NewCloudFunctionsAuthPolicy(ticket), NewCloudFunctionsRPCOAuthPolicy(token), signals)
	return &p
}

type JSONRPCServerMock struct {
	provider JSONRPCProvider
	server   *httptest.Server

	requests           map[string]adapter.JSONRPCRequest
	unlinkResponses    map[string]adapter.UnlinkResult
	discoveryResponses map[string]adapter.DiscoveryResult
	queryResponses     map[string]adapter.StatesResult
	actionResponses    map[string]adapter.ActionResult

	testToken  string
	testTicket string
}

func (p *JSONRPCServerMock) GetJSONRPCResponse(w http.ResponseWriter, r *http.Request) {
	XFunctionsServiceTicket := r.Header.Get("X-Functions-Service-Ticket")
	if XFunctionsServiceTicket == "" {
		http.Error(w, "", http.StatusUnauthorized)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	var req adapter.JSONRPCRequest
	err = json.Unmarshal(body, &req)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	p.requests[req.Headers.RequestID] = req

	var res []byte

	switch req.RequestType {
	case adapter.Unlink:
		res, err = json.Marshal(p.unlinkResponses[req.Headers.RequestID])
	case adapter.Discovery:
		res, err = json.Marshal(p.discoveryResponses[req.Headers.RequestID])
	case adapter.Query:
		res, err = json.Marshal(p.queryResponses[req.Headers.RequestID])
	case adapter.Action:
		res, err = json.Marshal(p.actionResponses[req.Headers.RequestID])
	}
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	w.WriteHeader(http.StatusOK)
	_, err = w.Write(res)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
}

func NewJSONRPCServerMock() *JSONRPCServerMock {
	p := JSONRPCServerMock{
		requests:   make(map[string]adapter.JSONRPCRequest),
		testToken:  "testToken",
		testTicket: "testTicket",
	}

	server := httptest.NewServer(http.HandlerFunc(p.GetJSONRPCResponse))
	p.server = server
	p.provider = *NewJSONRPCProviderMock(server.URL, "testName", "Тестовое имя", "testID", p.testToken, p.testTicket)

	return &p
}

type ActionSignalsMock struct {
	requestSignals             RequestSignals
	success                    metrics.CounterMock
	unknownError               metrics.CounterMock
	totalRequests              metrics.CounterMock
	errorCodesToMetricsCounter map[adapter.ErrorCode]metrics.CounterMock
}

func (as ActionSignalsMock) getErrorCounter(errorCode adapter.ErrorCode) metrics.CounterMock {
	if _, exist := KnownErrorCodesToSolomonCommandStatus[errorCode]; !exist {
		return as.unknownError
	}
	if mCounter, exist := as.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	as.errorCodesToMetricsCounter[errorCode] = metrics.CounterMock{}
	return as.errorCodesToMetricsCounter[errorCode]
}

func (as ActionSignalsMock) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		as.getErrorCounter(errorCode).Add(int64(errorCount))
	}
	return totalErrors
}

func (as ActionSignalsMock) GetRequestSignals() RequestSignals {
	return as.requestSignals
}

func (as ActionSignalsMock) GetSuccess() coremetrics.Counter {
	return as.success
}

func (as ActionSignalsMock) GetTotalRequests() coremetrics.Counter {
	return as.totalRequests
}

type QuerySignalsMock struct {
	requestSignals             RequestSignals
	success                    metrics.CounterMock
	unknownError               metrics.CounterMock
	totalRequests              metrics.CounterMock
	errorCodesToMetricsCounter map[adapter.ErrorCode]metrics.CounterMock
}

func (qs QuerySignalsMock) getErrorCounter(errorCode adapter.ErrorCode) metrics.CounterMock {
	if _, exist := KnownErrorCodesToSolomonCommandStatus[errorCode]; !exist {
		return qs.unknownError
	}
	if mCounter, exist := qs.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	qs.errorCodesToMetricsCounter[errorCode] = metrics.CounterMock{}
	return qs.errorCodesToMetricsCounter[errorCode]
}

func (qs QuerySignalsMock) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		qs.getErrorCounter(errorCode).Add(int64(errorCount))
	}
	return totalErrors
}

func (qs QuerySignalsMock) GetRequestSignals() RequestSignals {
	return qs.requestSignals
}

func (qs QuerySignalsMock) GetSuccess() coremetrics.Counter {
	return qs.success
}

func (qs QuerySignalsMock) GetTotalRequests() coremetrics.Counter {
	return qs.totalRequests
}

func (p *JSONRPCServerMock) WithDiscoveryResponses(responses map[string]adapter.DiscoveryResult) *JSONRPCServerMock {
	p.discoveryResponses = responses
	return p
}

func (p *JSONRPCServerMock) WithQueryResponses(responses map[string]adapter.StatesResult) *JSONRPCServerMock {
	p.queryResponses = responses
	return p
}

func (p *JSONRPCServerMock) WithActionResponses(responses map[string]adapter.ActionResult) *JSONRPCServerMock {
	p.actionResponses = responses
	return p
}

func (p *JSONRPCServerMock) WithUnlinkResponses(responses map[string]adapter.UnlinkResult) *JSONRPCServerMock {
	p.unlinkResponses = responses
	return p
}

func (p *JSONRPCServerMock) WithTestToken(token string) *JSONRPCServerMock {
	p.testToken = token
	return p
}

func (p *JSONRPCServerMock) WithTestTicket(ticket string) *JSONRPCServerMock {
	p.testTicket = ticket
	return p
}

func NewRequestSignalsMock() RequestSignals {
	return RequestSignals{
		ok:               metrics.CounterMock{},
		errorHTTP3xx:     metrics.CounterMock{},
		errorHTTP4xx:     metrics.CounterMock{},
		errorHTTP5xx:     metrics.CounterMock{},
		errorTimeout:     metrics.CounterMock{},
		errorBadResponse: metrics.CounterMock{},
		errorOther:       metrics.CounterMock{},
		total:            metrics.CounterMock{},
		duration:         metrics.TimerMock{},
	}
}

func NewDiscoverySignalsMock() DiscoverySignals {
	return DiscoverySignals{
		RequestSignals:  NewRequestSignalsMock(),
		success:         metrics.CounterMock{},
		validationError: metrics.CounterMock{},
		totalRequests:   metrics.CounterMock{},
	}
}

func NewQuerySignalsMock() IQuerySignals {
	return QuerySignalsMock{
		requestSignals:             NewRequestSignalsMock(),
		success:                    metrics.CounterMock{},
		unknownError:               metrics.CounterMock{},
		totalRequests:              metrics.CounterMock{},
		errorCodesToMetricsCounter: make(map[adapter.ErrorCode]metrics.CounterMock),
	}
}

func NewActionSignalsMock() IActionSignals {
	return ActionSignalsMock{
		requestSignals:             NewRequestSignalsMock(),
		success:                    metrics.CounterMock{},
		unknownError:               metrics.CounterMock{},
		totalRequests:              metrics.CounterMock{},
		errorCodesToMetricsCounter: make(map[adapter.ErrorCode]metrics.CounterMock),
	}
}

func NewHubRemotesSignalsMock() HubRemotesSignals {
	return HubRemotesSignals{
		RequestSignals: NewRequestSignalsMock(),
		success:        nil,
		deviceNotFound: nil,
		unknownError:   nil,
		totalRequests:  nil,
	}
}

func NewDeleteSignalsMock() DeleteSignals {
	return DeleteSignals{
		RequestSignals: NewRequestSignalsMock(),
		success:        nil,
		deviceNotFound: nil,
		unknownError:   nil,
		totalRequests:  nil,
	}
}

func NewSkillSignalsMock() Signals {
	return Signals{
		discovery:  NewDiscoverySignalsMock(),
		query:      NewQuerySignalsMock(),
		action:     NewActionSignalsMock(),
		unlink:     NewRequestSignalsMock(),
		hubRemotes: NewHubRemotesSignalsMock(),
		delete:     NewDeleteSignalsMock(),
		rename:     NewRequestSignalsMock(),
	}
}
