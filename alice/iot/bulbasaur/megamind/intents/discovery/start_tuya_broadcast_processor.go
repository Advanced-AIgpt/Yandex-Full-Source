package discovery

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging/doublelog"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var StartTuyaBroadcastProcessorName = "start_tuya_broadcast_processor"

type StartTuyaBroadcastProcessor struct {
	logger              log.Logger
	tuyaDiscoveryClient *tuyaDiscoveryClient
	dbClient            db.DB
}

func NewStartTuyaBroadcastProcessor(l log.Logger, tuyaClient tuyaclient.IClient, dbClient db.DB) *StartTuyaBroadcastProcessor {
	return &StartTuyaBroadcastProcessor{logger: l, tuyaDiscoveryClient: newTuyaDiscoveryClient(l, tuyaClient, dbClient), dbClient: dbClient}
}

func (p *StartTuyaBroadcastProcessor) Name() string {
	return StartTuyaBroadcastProcessorName
}

func (p *StartTuyaBroadcastProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.StartTuyaBroadcastFrameName,
		},
	}
}

func (p *StartTuyaBroadcastProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p StartTuyaBroadcastProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, p.supportedApplyArguments())
}

func (p StartTuyaBroadcastProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p StartTuyaBroadcastProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(StartTuyaBroadcastApplyArguments)
}

func (p *StartTuyaBroadcastProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.StartTuyaBroadcastFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}
	var state State
	if ctx.Request().GetBaseRequest().GetState() != nil {
		if err := state.FromProtoState(ctx.Request().GetBaseRequest().GetState()); err != nil {
			ctx.Logger().Errorf("failed to unmarshal request state: %v", err)
		}
	} else {
		ctx.Logger().Info("no state found in request, skip state unmarshalling")
	}
	ctx.Logger().Info("got run with frame", log.Any("frame", frame))
	args := &StartTuyaBroadcastApplyArguments{
		SSID:      frame.SSID,
		Password:  frame.Password,
		SessionID: state.SessionID,
	}
	return sdk.RunResponseBuilder(ctx, args)
}

func (p StartTuyaBroadcastProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*StartTuyaBroadcastApplyArguments)
	user, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to start broadcast: user not authenticated")
	}

	internetConnection := libmegamind.InternetConnection{
		TDeviceState_TInternetConnection: ctx.Request().GetBaseRequest().GetDeviceState().GetInternetConnection(),
	}

	ctx.Logger().Info("current session id", log.Any("session_id", args.SessionID))

	currentSSID := args.SSID
	password := args.Password
	suitableNetwork, suitableNetworkExists := internetConnection.GetSuitableNetwork()
	if suitableNetworkExists {
		currentSSID = suitableNetwork.GetSsid()
		// todo: check active protocols on error -> say error if wifi is the only one
	}
	tokenRequest := client.GetTokenRequest{
		SSID:           currentSSID,
		Password:       password,
		ConnectionType: tuya.EzMode,
	}
	tokenResponse, err := p.tuyaDiscoveryClient.tuyaClient.GetToken(ctx.Context(), user.ID, tokenRequest)
	if err != nil {
		return nil, xerrors.Errorf("failed to get tuya token: %w", err)
	}

	go goroutines.SafeBackground(contexter.NoCancel(ctx.Context()), p.logger, func(insideCtx context.Context) {
		doublelog.Info(insideCtx, p.logger, "found active pairing token, will try tuya discovery in background")
		// tuya discovery goes in background and saves intent state with discovered devices to db
		// finish discovery acquires them from db later
		if err := p.tuyaDiscoveryClient.DiscoverTuyaDevices(insideCtx, user.ID, ctx.ClientDeviceID(), args.SessionID, tokenResponse.TokenInfo.Token); err != nil {
			doublelog.Errorf(insideCtx, p.logger, "failed to discover tuya devices: %v", err)
		}
	})
	startTuyaBroadcastDirective := StartTuyaBroadcastDirective{
		SSID:         currentSSID,
		Password:     password,
		Cipher:       tokenResponse.TokenInfo.Cipher,
		PairingToken: tokenResponse.TokenInfo.GetPairingToken(),
	}
	// IOT-1477: start tuya broadcast overwrites nlu hints from start processor
	cancelCallback := CancelCallback{}
	// IOT-1607: also any response with no state in it overwrites old megamind state so we should resave it
	state := State{SessionID: args.SessionID}
	statepb, err := anypb.New(state.toProto())
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal state: %w", err)
	}
	return libmegamind.NewApplyResponse(IoTScenarioName, DiscoveryIntent).
		WithDirectives(startTuyaBroadcastDirective.ToDirective(ctx.ClientDeviceID())).
		WithCallbackFrameAction(cancelCallback.ToCallbackFrameAction()).
		WithState(statepb).
		Build(), nil
}
