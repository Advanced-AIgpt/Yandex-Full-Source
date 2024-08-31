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

var DeleteNetworksProcessorName = "delete_networks_processor"

type DeleteNetworksProcessor struct {
	logger log.Logger
	db     db.DB
}

func NewDeleteNetworksProcessor(l log.Logger, db db.DB) *DeleteNetworksProcessor {
	return &DeleteNetworksProcessor{logger: l, db: db}
}

func (p *DeleteNetworksProcessor) Name() string {
	return DeleteNetworksProcessorName
}

func (p *DeleteNetworksProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.DeleteNetworksTypedSemanticFrame,
		},
	}
}

func (p *DeleteNetworksProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *DeleteNetworksProcessor) IsApplicable(args *anypb.Any) bool {
	// note: if we agree that all frame processors use UniversalArguments, IsApplicable is not needed.
	return sdk.IsApplyArguments(args, new(DeleteNetworksApplyArguments))
}

func (p *DeleteNetworksProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *DeleteNetworksProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame DeleteNetworksFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}

	ctx.Logger().Info("got run request with frame", log.Any("frame", frame))

	originDevice, err := ctx.OriginDevice()
	if err != nil {
		return nil, xerrors.Errorf("failed to get origin device: %w", err)
	}

	args := &DeleteNetworksApplyArguments{
		SpeakerID: originDevice.ID,
	}

	// note: this serialization should live inside sdk.responseBuilders
	protoArgs, err := sdk.MarshalApplyArguments(args)
	if err != nil {
		return nil, err
	}

	return libmegamind.NewRunResponse(IoTScenarioName, NetworksIntent).
		WithApplyArguments(protoArgs).Build(), nil
}

func (p *DeleteNetworksProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(DeleteNetworksApplyArguments)
}

func (p *DeleteNetworksProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*DeleteNetworksApplyArguments)

	user, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to delete networks: user not authorized")
	}
	err := p.deleteNetworks(ctx.Context(), user.ID, args.SpeakerID)
	if err != nil {
		return nil, xerrors.Errorf("failed to delete networks: %w", err)
	}
	directive := DeleteNetworksDirective{}
	scenarioDirective := directive.ToDirective(ctx.ClientDeviceID())
	return libmegamind.NewApplyResponse(IoTScenarioName, NetworksIntent).
		WithDirectives(scenarioDirective).Build(), nil
}

func (p *DeleteNetworksProcessor) deleteNetworks(ctx context.Context, userID uint64, speakerID string) error {
	return p.db.Transaction(ctx, "delete_speaker_networks_tx", func(ctx context.Context) error {
		speaker, err := p.db.SelectUserDevice(ctx, userID, speakerID)
		if err != nil {
			return xerrors.Errorf("failed to select speaker: %w", err)
		}
		if speaker.InternalConfig.SpeakerYandexIOConfig == nil {
			return nil
		}
		speaker.InternalConfig.SpeakerYandexIOConfig = nil
		return p.db.StoreUserDeviceConfigs(ctx, userID, map[string]model.DeviceConfig{
			speaker.ID: speaker.InternalConfig,
		})
	})
}
