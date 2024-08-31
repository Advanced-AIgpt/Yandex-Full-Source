package networks

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var RestoreNetworksProcessorName = "restore_networks_processor"

type RestoreNetworksProcessor struct {
	logger log.Logger
	db     db.DB
}

func NewRestoreNetworksProcessor(l log.Logger, db db.DB) *RestoreNetworksProcessor {
	return &RestoreNetworksProcessor{logger: l, db: db}
}

func (p *RestoreNetworksProcessor) Name() string {
	return RestoreNetworksProcessorName
}

func (p *RestoreNetworksProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.RestoreNetworksTypedSemanticFrame,
		},
	}
}

func (p *RestoreNetworksProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *RestoreNetworksProcessor) IsApplicable(args *anypb.Any) bool {
	// note: if we agree that all frame processors use UniversalArguments, IsApplicable is not needed.
	return sdk.IsApplyArguments(args, new(RestoreNetworksApplyArguments))
}

func (p *RestoreNetworksProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	// if we use ApplyContext and UniversalApplyArgumnts in all frame processors, we can
	// - omit passing logger to all processors
	// - omit isApplicable
	// - parse and log arguments for processor automatically

	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *RestoreNetworksProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame RestoreNetworksFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}

	ctx.Logger().Info("got run request with frame", log.Any("frame", frame))

	originDevice, err := ctx.OriginDevice()
	if err != nil {
		return nil, xerrors.Errorf("failed to get origin device: %w", err)
	}

	args := &RestoreNetworksApplyArguments{
		SpeakerID: originDevice.ID,
	}

	// note: this serialization should live inside sdk.responseBuilders
	protoArgs, err := sdk.MarshalApplyArguments(args)
	if err != nil {
		return nil, err
	}

	return libmegamind.NewRunResponse(IoTScenarioName, NetworksIntent).WithApplyArguments(protoArgs).Build(), nil
}

func (p *RestoreNetworksProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(RestoreNetworksApplyArguments)
}

func (p *RestoreNetworksProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*RestoreNetworksApplyArguments)

	user, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to restore networks: user not authorized")
	}
	restoredNetworks, err := p.restoreNetworks(ctx.Context(), user.ID, args.SpeakerID)
	if err != nil {
		return nil, xerrors.Errorf("failed to restore networks: %w", err)
	}

	directive := RestoreNetworksDirective{Networks: restoredNetworks}
	scenarioDirective := directive.ToDirective(ctx.ClientDeviceID())
	return libmegamind.NewApplyResponse(IoTScenarioName, NetworksIntent).WithDirectives(scenarioDirective).Build(), nil
}

func (p *RestoreNetworksProcessor) restoreNetworks(ctx context.Context, userID uint64, speakerID string) (*model.SpeakerNetworks, error) {
	speaker, err := p.db.SelectUserDevice(ctx, userID, speakerID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select speaker: %w", err)
	}
	if speaker.InternalConfig.SpeakerYandexIOConfig != nil {
		return &speaker.InternalConfig.SpeakerYandexIOConfig.Networks, nil
	}
	return nil, nil
}
