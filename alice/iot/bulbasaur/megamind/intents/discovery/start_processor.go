package discovery

import (
	"context"
	"strings"

	"github.com/gofrs/uuid"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

var StartDiscoveryProcessorName = "start_discovery_processor"

type StartDiscoveryProcessor struct {
	logger         log.Logger
	linksGenerator sup.AppLinksGenerator
	DB             db.DB
}

func NewStartDiscoveryProcessor(l log.Logger, appLinksGenerator sup.AppLinksGenerator, dbClient db.DB) *StartDiscoveryProcessor {
	return &StartDiscoveryProcessor{
		logger:         l,
		linksGenerator: appLinksGenerator,
		DB:             dbClient,
	}
}

func (p *StartDiscoveryProcessor) Name() string {
	return StartDiscoveryProcessorName
}

func (p *StartDiscoveryProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.StartDiscoveryFrameName,
			frames.StartSearchFrameName,
		},
	}
}

func (p *StartDiscoveryProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *StartDiscoveryProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	inputFrame := input.GetFirstFrame()
	if inputFrame == nil {
		return megamind.Irrelevant(ctx.Context(), p.logger, megamind.UnknownFrame)
	}
	ctx.Logger().Info("input data sources", log.Any("data_sources", ctx.Request().GetDataSources()))

	var (
		protocols     model.Protocols
		sessionID     *string
		discoveryType = discovery.FastDiscoveryType
		deviceType    = model.OtherDeviceType
		skillID       *string
	)

	switch inputFrame.Name() {
	case string(frames.StartSearchFrameName):
		protocols = p.extractProtocolsFromRunContext(ctx)
		discoveryType = p.extractDiscoveryTypeFromSemanticFrame(inputFrame)
		skillID = p.extractSkillIDFromSemanticFrame(inputFrame)
		deviceType = p.extractDeviceTypeFromSemanticFrame(inputFrame)
	case string(frames.StartDiscoveryFrameName):
		var frame frames.StartDiscoveryFrame
		if err := sdk.UnmarshalTSF(input, &frame); err != nil {
			return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
		}
		protocols = frame.Protocols
		sessionID = ptr.String(frame.SessionID)
		intentState := getIntentState(ctx, p.DB, frame.SessionID)
		if intentState.DiscoveryType != "" {
			discoveryType = intentState.DiscoveryType
		}
	default:
		return megamind.Irrelevant(ctx.Context(), p.logger, megamind.UnknownFrame)
	}
	args := &StartDiscoveryApplyArguments{
		SessionID:     sessionID,
		DiscoveryType: &discoveryType,
		DeviceType:    &deviceType,
		SkillID:       skillID,
	}
	args.CalculateProtocolSupports(ctx, protocols)
	return sdk.RunResponseBuilder(ctx, args)
}

func (p *StartDiscoveryProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(StartDiscoveryApplyArguments))
}

func (p *StartDiscoveryProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *StartDiscoveryProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(StartDiscoveryApplyArguments)
}

func (p *StartDiscoveryProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*StartDiscoveryApplyArguments)

	_, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to start discovery: user not authenticated")
	}

	deviceType := model.OtherDeviceType
	if args.DeviceType != nil {
		deviceType = *args.DeviceType
	}

	skillID := model.YANDEXIO
	if args.SkillID != nil {
		skillID = *args.SkillID
	}

	// mobile discovery
	switch {
	case ctx.ClientInfo().IsSearchApp():
		directive := OpenURIDirective{URI: sup.SearchAppLink(p.linksGenerator.BuildAddDevicePageLink(deviceType, skillID))}
		return sdk.ApplyResponseBuilder(ctx, okNLG, directive.ToDirective(ctx.ClientDeviceID())), nil
	case ctx.ClientInfo().IsIotAppAndroid():
		directive := OpenURIDirective{URI: sup.IotAppAndroidLink(p.linksGenerator.BuildAddDevicePageLink(deviceType, skillID))}
		return sdk.ApplyResponseBuilder(ctx, okNLG, directive.ToDirective(ctx.ClientDeviceID())), nil
	case ctx.ClientInfo().IsIotAppIOS():
		directive := OpenURIDirective{URI: sup.IotAppIOSLink(p.linksGenerator.BuildAddDevicePageLink(deviceType, skillID))}
		return sdk.ApplyResponseBuilder(ctx, okNLG, directive.ToDirective(ctx.ClientDeviceID())), nil
	}

	// speaker discovery
	switch {
	case !args.AccountSupport():
		return sdk.ApplyResponseBuilder(ctx, discoveryUnavailableNLG), nil
	case !args.ClientSupport():
		return sdk.ApplyResponseBuilder(ctx, unsupportedClientNLG), nil
	case len(args.Protocols()) == 0:
		return sdk.ApplyResponseBuilder(ctx, unsupportedClientNLG), nil
	}

	var sessionID string
	if args.SessionID != nil {
		sessionID = *args.SessionID
	} else {
		sessionID = uuid.Must(uuid.NewV4()).String()
	}

	ctx.Logger().Info("current session id", log.Any("session_id", sessionID))

	directive := StartDiscoveryDirective{
		Protocols: args.Protocols(),
	}

	discoveryType := discovery.FastDiscoveryType
	if args.DiscoveryType != nil {
		discoveryType = *args.DiscoveryType
	}
	switch discoveryType {
	case discovery.SlowDiscoveryType:
		directive.DeviceLimit = 0
	default:
		directive.DeviceLimit = 1
	}

	scenarioDirective := directive.ToDirective(ctx.ClientDeviceID())

	ctx.Logger().Info("start_discovery_directive", log.Any("directive", scenarioDirective))

	cancelCallback := CancelCallback{}
	nlgAsset := startSearchNLG.RandomAsset(ctx.Context())

	state := State{SessionID: sessionID}
	statepb, err := anypb.New(state.toProto())
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal state: %w", err)
	}

	return libmegamind.NewApplyResponse(IoTScenarioName, DiscoveryIntent).
		WithCallbackFrameAction(cancelCallback.ToCallbackFrameAction()).
		WithLayout(nlgAsset.Text(), nlgAsset.Speech(), scenarioDirective).
		WithState(statepb).Build(), nil
}

func (p *StartDiscoveryProcessor) extractProtocolsFromRunContext(ctx sdk.RunContext) model.Protocols {
	result := make(model.Protocols, 0)
	dataSource, ok := ctx.Request().GetDataSources()[int32(commonpb.EDataSourceType_ENVIRONMENT_STATE)]
	if !ok {
		ctx.Logger().Info("ENVIRONMENT_STATE datasource not found")
		return result
	}
	for _, endpoint := range dataSource.GetEnvironmentState().GetEndpoints() {
		if endpoint.GetId() != ctx.ClientInfo().DeviceID {
			continue
		}
		for _, c := range endpoint.GetCapabilities() {
			var (
				iotDiscoveryCapabilityMessage = new(endpointpb.TIotDiscoveryCapability)
			)
			switch {
			case c.MessageIs(iotDiscoveryCapabilityMessage):
				if err := c.UnmarshalTo(iotDiscoveryCapabilityMessage); err != nil {
					continue
				}
				result.FromProto(iotDiscoveryCapabilityMessage.GetParameters().GetSupportedProtocols())
				return result
			}
		}
	}
	return result
}

const (
	DeviceTypeSlotName    string = "device_type"
	DiscoveryTypeSlotName string = "discovery_type"
	ProviderSlotName      string = "provider"
)

func (p *StartDiscoveryProcessor) extractDiscoveryTypeFromSemanticFrame(semanticFrame *libmegamind.SemanticFrame) discovery.DiscoveryType {
	deviceType := model.LightDeviceType
	discoveryType := discovery.FastDiscoveryType
	for _, slot := range semanticFrame.Slots() {
		if slot.IsEmpty() {
			continue
		}
		switch slot.Name {
		case DeviceTypeSlotName:
			deviceType = model.DeviceType(slot.Value)
		case DiscoveryTypeSlotName:
			discoveryType = discovery.DiscoveryType(slot.Value)
		}
	}
	if deviceType == model.LightCeilingDeviceType {
		return discovery.SlowDiscoveryType
	}
	return discoveryType
}

func (p *StartDiscoveryProcessor) extractDeviceTypeFromSemanticFrame(semanticFrame *libmegamind.SemanticFrame) model.DeviceType {
	for _, slot := range semanticFrame.Slots() {
		if slot.IsEmpty() {
			continue
		}
		if slot.Name == DeviceTypeSlotName {
			return model.DeviceType(slot.Value)
		}
	}
	return model.OtherDeviceType
}

func (p *StartDiscoveryProcessor) extractSkillIDFromSemanticFrame(semanticFrame *libmegamind.SemanticFrame) *string {
	for _, slot := range semanticFrame.Slots() {
		if slot.IsEmpty() {
			continue
		}
		if slot.Name == ProviderSlotName {
			if strings.EqualFold(slot.Value, "Yandex") {
				return ptr.String(model.TUYA)
			}
		}
	}
	return nil
}
