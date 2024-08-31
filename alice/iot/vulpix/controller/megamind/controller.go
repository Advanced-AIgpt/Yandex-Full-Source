package megamind

import (
	"context"
	"fmt"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	steelixclient "a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/iot/vulpix/controller/notificator"
	"a.yandex-team.ru/alice/iot/vulpix/controller/sup"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/nlg"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/library/go/setrace"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger         log.Logger
	Sup            sup.IController
	PushController IPushController
	Tuya           tuyaclient.IClient
	Steelix        steelixclient.IClient
	DB             db.IClient

	runProcessors      []RunProcessor
	applyProcessors    []ApplyProcessor
	continueProcessors []ContinueProcessor
}

func newController(logger log.Logger, sup sup.IController, notificator IPushController, tuyaClient tuyaclient.IClient,
	steelixClient steelixclient.IClient, dbClient db.IClient) *Controller {
	return &Controller{
		Logger:             logger,
		Sup:                sup,
		PushController:     notificator,
		Tuya:               tuyaClient,
		Steelix:            steelixClient,
		DB:                 dbClient,
		runProcessors:      make([]RunProcessor, 0),
		applyProcessors:    make([]ApplyProcessor, 0),
		continueProcessors: make([]ContinueProcessor, 0),
	}
}

func NewController(metrics *solomon.Registry, notificatorClient libnotificator.IClient, supClient libsup.IClient,
	dbClient db.IClient, tuyaClient tuyaclient.IClient, steelixClient steelixclient.IClient, logger log.Logger) *Controller {
	//bassController := &bass.Controller{Bass: bassClient}
	notificatorController := &notificator.Controller{Notificator: notificatorClient}
	supController := &sup.Controller{Sup: supClient, Logger: logger}

	lampFinder := DeviceFinder{Logger: logger, DB: dbClient, Tuya: tuyaClient, Steelix: steelixClient}
	startRunProc := &StartRunProcessorV1{Logger: logger}
	startApplyProc1 := &StartStep1ApplyProcessorV1{Logger: logger, PushController: notificatorController, DeviceFinder: lampFinder}
	startApplyProc2 := &StartStep2ApplyProcessorV1{Logger: logger, PushController: notificatorController, DeviceFinder: lampFinder}
	cancelProc := &CancelProcessor{Logger: logger, DB: dbClient}
	failureRunProc := &FailureRunProcessor{Logger: logger, DB: dbClient}
	failureApplyProc1 := &FailureStep1ApplyProcessor{Logger: logger, DB: dbClient}
	failureApplyProc2 := &FailureStep2ApplyProcessor{Logger: logger, Sup: supController, DB: dbClient}

	failureRunV2 := &FailureRunProcessorV2{DB: dbClient, Logger: logger}
	failureStep1ApplyV2 := &FailureStep1ApplyProcessorV2{Logger: logger}
	failureStep2ApplyV2 := &FailureStep2ApplyProcessorV2{Logger: logger, Sup: supController, DB: dbClient}
	startRunV2 := &StartRunProcessorV2{Logger: logger}
	startStep1V2 := &StartStep1ContinueProcessorV2{Logger: logger, PushController: notificatorController, Tuya: tuyaClient, DeviceFinder: lampFinder}
	startStep2V2 := &StartStep2ContinueProcessorV2{Logger: logger, Bass: notificatorController, Tuya: tuyaClient, DeviceFinder: lampFinder}
	successV2 := &SuccessProcessorV2{Logger: logger, DB: dbClient}

	connectProc := &ConnectProcessor{Logger: logger, Sup: supController, ClientSignals: NewClientSignals(metrics.WithPrefix("connect")), DB: dbClient}

	connectProcWithMetrics := NewRunApplyProcessorWithMetrics(connectProc, connectProc, metrics, "connect")
	connect2ProcWithMetrics := NewRunProcessorWithMetrics(&ConnectStep2Processor{Logger: logger}, metrics, "connect_2")
	successProcWithMetrics := NewRunProcessorWithMetrics(&SuccessProcessorV1{Logger: logger, DB: dbClient}, metrics, "success")
	failureRunProcWithMetrics := NewRunProcessorWithMetrics(failureRunProc, metrics, "failure_run")
	failureApplyProc1WithMetrics := NewApplyProcessorWithMetrics(failureApplyProc1, metrics, "failure_apply_1")
	failureApplyProc2WithMetrics := NewApplyProcessorWithMetrics(failureApplyProc2, metrics, "failure_apply_2")
	startRunProcWithMetrics := NewRunProcessorWithMetrics(startRunProc, metrics, "start_run")
	startApplyProc1WithMetrics := NewApplyProcessorWithMetrics(startApplyProc1, metrics, "start_apply")
	startApplyProc2WithMetrics := NewApplyProcessorWithMetrics(startApplyProc2, metrics, "start_apply_2")
	cancelProcWithMetrics := NewRunApplyProcessorWithMetrics(cancelProc, cancelProc, metrics, "cancel")
	howToProcWithMetrics := NewRunProcessorWithMetrics(&HowToProcessor{Logger: logger}, metrics, "how_to")

	// v2
	failureRunV2WithMetrics := NewRunProcessorWithMetrics(failureRunV2, metrics, "failure_run_v2")
	failureStep1ApplyV2WithMetrics := NewApplyProcessorWithMetrics(failureStep1ApplyV2, metrics, "failure_apply_1_v2")
	failureStep2ApplyV2WithMetrics := NewApplyProcessorWithMetrics(failureStep2ApplyV2, metrics, "failure_apply_2_v2")
	startRunV2WithMetrics := NewRunProcessorWithMetrics(startRunV2, metrics, "start_run_v2")
	startStep1V2WithMetrics := NewContinueProcessorWithMetrics(startStep1V2, metrics, "start_apply_v2")
	startStep2V2WithMetrics := NewContinueProcessorWithMetrics(startStep2V2, metrics, "start_apply_2_v2")
	successV2WithMetrics := NewRunProcessorWithMetrics(successV2, metrics, "success_v2")

	return newController(logger, supController, notificatorController, tuyaClient, steelixClient, dbClient).
		WithProcessor(connectProcWithMetrics).
		WithRunProcessor(connect2ProcWithMetrics).
		WithRunProcessor(successProcWithMetrics).
		WithProcessor(cancelProcWithMetrics).
		WithRunProcessor(failureRunProcWithMetrics).
		WithApplyProcessor(failureApplyProc1WithMetrics).
		WithApplyProcessor(failureApplyProc2WithMetrics).
		WithRunProcessor(startRunProcWithMetrics).
		WithApplyProcessor(startApplyProc1WithMetrics).
		WithApplyProcessor(startApplyProc2WithMetrics).
		WithRunProcessor(howToProcWithMetrics).
		WithRunProcessor(failureRunV2WithMetrics).
		WithApplyProcessor(failureStep1ApplyV2WithMetrics).
		WithApplyProcessor(failureStep2ApplyV2WithMetrics).
		WithRunProcessor(startRunV2WithMetrics).
		WithContinueProcessor(startStep1V2WithMetrics).
		WithContinueProcessor(startStep2V2WithMetrics).
		WithRunProcessor(successV2WithMetrics)
}

func (c *Controller) WithProcessor(p RunApplyProcessor) *Controller {
	return c.WithRunProcessor(p).WithApplyProcessor(p)
}

func (c *Controller) WithRunProcessor(p RunProcessor) *Controller {
	c.runProcessors = append(c.runProcessors, p)
	return c
}

func (c *Controller) WithApplyProcessor(p ApplyProcessor) *Controller {
	c.applyProcessors = append(c.applyProcessors, p)
	return c
}

func (c *Controller) WithContinueProcessor(p ContinueProcessor) *Controller {
	c.continueProcessors = append(c.continueProcessors, p)
	return c
}

func (c *Controller) Name() string {
	return model.IoTVoiceDiscoveryScenarioIntent
}

func (c *Controller) IsRunnable(ctx context.Context, request *scenarios.TScenarioRunRequest) bool {
	if c.ClientHasIotDiscoveryCapability(ctx, request) {
		return false
	}
	for _, p := range c.runProcessors {
		if p.IsRunnable(request) {
			return true
		}
	}
	return false
}

func (c *Controller) IsApplicable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool {
	var applyArguments protos.TApplyArguments // apply request always has not nil apply argument; otherwise it's not apply request
	if err := request.Arguments.UnmarshalTo(&applyArguments); err != nil {
		ctxlog.Debugf(ctx, c.Logger, "cannot unmarshal apply_arguments: %s", err)
		return false
	}
	for _, p := range c.applyProcessors {
		if p.IsApplicable(request, &applyArguments) {
			return true
		}
	}
	return false
}

func (c *Controller) IsContinuable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool {
	var continueArguments protos.TContinueArguments // continue request always has not nil continue argument; otherwise it's not continue request
	if err := request.Arguments.UnmarshalTo(&continueArguments); err != nil {
		ctxlog.Debugf(ctx, c.Logger, "cannot unmarshal continue_arguments: %s", err)
		return false
	}
	for _, p := range c.continueProcessors {
		if p.IsContinuable(request, &continueArguments) {
			return true
		}
	}
	return false
}

func (c *Controller) Run(ctx context.Context, runRequest *scenarios.TScenarioRunRequest, user bmodel.User) (*scenarios.TScenarioRunResponse, error) {
	if runRequest == nil {
		return irrelevant(ctx, c.Logger)
	}
	if dataSource, dataSourceOk := runRequest.DataSources[int32(common.EDataSourceType_BLACK_BOX)]; dataSourceOk {
		userInfo := libmegamind.NewUserInfo(dataSource.GetUserInfo())
		expManager, err := experiments.ManagerFromContext(ctx)
		if err != nil {
			return nil, xerrors.Errorf("unable to get experiments manager: %w", err)
		}
		expManager.SetStaff(user.ID, userInfo.IsStaff)
	}
	clientInfo := libmegamind.NewClientInfo(runRequest.BaseRequest.ClientInfo)

	if user.IsEmpty() {
		if clientInfo.IsElariWatch() {
			return nlgRunResponse(ctx, c.Logger, nlgUnsupportedClient)
		}
		if !clientInfo.IsWindows() && clientInfo.IsYaMessenger() { // https://st.yandex-team.ru/ALICE-7178
			return nlgRunResponse(ctx, c.Logger, nlgUnsupportedClient)
		}
		return nlgRunResponse(ctx, c.Logger, nlgNeedLogin)
	}

	for _, p := range c.runProcessors {
		if p.IsRunnable(runRequest) {
			message := fmt.Sprintf("`%s` processor has been chosen", p.Name())
			ctxlog.Infof(ctx, c.Logger, message)
			setrace.InfoLogEvent(ctx, c.Logger, message)
			return p.Run(ctx, user.ID, runRequest)
		}
	}

	return irrelevant(ctx, c.Logger)
}

func (c *Controller) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user bmodel.User) (*scenarios.TScenarioApplyResponse, error) {
	if applyRequest == nil {
		return nlgApplyResponse(ctx, c.Logger, nlgCommonError)
	}

	var applyArguments protos.TApplyArguments // apply request always has not nil apply argument; otherwise it's not apply request
	if err := applyRequest.Arguments.UnmarshalTo(&applyArguments); err != nil {
		ctxlog.Warnf(ctx, c.Logger, "cannot unmarshal apply_arguments: %s", err)
		return nlgApplyResponse(ctx, c.Logger, nlgCommonError)
	}

	for _, p := range c.applyProcessors {
		if p.IsApplicable(applyRequest, &applyArguments) {
			return p.Apply(ctx, user.ID, applyRequest, &applyArguments)
		}
	}

	ctxlog.Warn(ctx, c.Logger, "Not callback type of input in apply handler: answer with commonError")
	return nlgApplyResponse(ctx, c.Logger, nlgCommonError)
}

func (c *Controller) Continue(ctx context.Context, continueRequest *scenarios.TScenarioApplyRequest, user bmodel.User) (*scenarios.TScenarioContinueResponse, error) {
	if continueRequest == nil {
		return nlgContinueResponse(ctx, c.Logger, nlgCommonError)
	}

	var continueArguments protos.TContinueArguments // continue request always has not nil continue argument; otherwise it's not continue request
	if err := continueRequest.Arguments.UnmarshalTo(&continueArguments); err != nil {
		ctxlog.Warnf(ctx, c.Logger, "cannot unmarshal continue_arguments: %s", err)
		return nlgContinueResponse(ctx, c.Logger, nlgCommonError)
	}

	for _, p := range c.continueProcessors {
		if p.IsContinuable(continueRequest, &continueArguments) {
			return p.Continue(ctx, user.ID, continueRequest, &continueArguments)
		}
	}

	ctxlog.Warn(ctx, c.Logger, "Not callback type of input in continue handler: answer with commonError")
	return nlgContinueResponse(ctx, c.Logger, nlgCommonError)
}

func (c *Controller) ClientHasIotDiscoveryCapability(ctx context.Context, request *scenarios.TScenarioRunRequest) bool {
	dataSource, ok := request.GetDataSources()[int32(common.EDataSourceType_ENVIRONMENT_STATE)]
	if !ok {
		ctxlog.Info(ctx, c.Logger, "ENVIRONMENT_STATE datasource not found")
		return false
	}
	clientDeviceID := request.GetBaseRequest().GetClientInfo().GetDeviceId()
	for _, endpoint := range dataSource.GetEnvironmentState().GetEndpoints() {
		if endpoint.GetId() != clientDeviceID {
			continue
		}
		for _, c := range endpoint.GetCapabilities() {
			var iotDiscoveryCapabilityMessage = new(endpointpb.TIotDiscoveryCapability)
			switch {
			case c.MessageIs(iotDiscoveryCapabilityMessage):
				return true
			default:
				continue
			}
		}
	}
	return false
}

func nlgRunResponse(ctx context.Context, logger log.Logger, n libnlg.NLG) (*scenarios.TScenarioRunResponse, error) {
	asset := n.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "NlgRunResponse: `%s`", asset.Text())
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}

func nlgApplyResponse(ctx context.Context, logger log.Logger, n libnlg.NLG) (*scenarios.TScenarioApplyResponse, error) {
	asset := n.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "NlgApplyResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}

func nlgContinueResponse(ctx context.Context, logger log.Logger, n libnlg.NLG) (*scenarios.TScenarioContinueResponse, error) {
	asset := n.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "NlgContinueResponse: `%s`", asset.Text())
	response := libmegamind.NewContinueResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}

func irrelevant(ctx context.Context, logger log.Logger) (*scenarios.TScenarioRunResponse, error) {
	irrelevantAsset := nlgIrrelevant.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "returning irrelevant response")
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithIrrelevant().WithLayout(irrelevantAsset.Text(), irrelevantAsset.Speech())
	return response.Build(), nil
}

func searchAppResponse(ctx context.Context, logger log.Logger, deviceType bmodel.DeviceType) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlgSearchApp.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "SearchAppResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(), GetOpenURIDirective(sup.GetDiscoveryLink(deviceType), SearchAppClientInfoType))
	return response.Build(), nil
}

func iotAppResponse(ctx context.Context, logger log.Logger, deviceType bmodel.DeviceType, clientInfoType ClientInfoType) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlgSearchApp.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "IoTAppResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(), GetOpenURIDirective(sup.GetDiscoveryLink(deviceType), clientInfoType))
	return response.Build(), nil
}

func standaloneAliceResponse(ctx context.Context, logger log.Logger, deviceType bmodel.DeviceType) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlgSearchApp.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "StandaloneAliceResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(), GetOpenURIDirective(sup.GetDiscoveryLink(deviceType), StandaloneAliceClientInfoType))
	return response.Build(), nil
}

// TODO: removed JBL Speaker from allowed speakers until JBL update their firmware
func isAllowedSpeaker(clientInfo libmegamind.ClientInfo) bool {
	return clientInfo.IsYandexMiniSpeaker() || clientInfo.IsYandexStationSpeaker() ||
		clientInfo.IsYandexMicroSpeaker() || clientInfo.IsYandexMidiSpeaker()
}

func GetClientInfoType(clientInfo libmegamind.ClientInfo) ClientInfoType {
	switch {
	case clientInfo.IsSearchApp():
		return SearchAppClientInfoType
	case clientInfo.IsIotAppIOS():
		return IotAppIOSClientInfoType
	case clientInfo.IsIotAppAndroid():
		return IotAppAndroidClientInfoType
	case clientInfo.IsStandalone():
		return StandaloneAliceClientInfoType
	case clientInfo.IsSmartSpeaker() && !isAllowedSpeaker(clientInfo):
		return UnsupportedSpeakerClientInfoType
	case !isAllowedSpeaker(clientInfo):
		return UnsupportedClientClientInfoType
	default:
		return SpeakerClientInfoType
	}
}

func IsInternetConnectionValid(connection *common.TDeviceState_TInternetConnection) bool {
	return connection.GetType() == common.TDeviceState_TInternetConnection_Wifi_2_4GHz
}
