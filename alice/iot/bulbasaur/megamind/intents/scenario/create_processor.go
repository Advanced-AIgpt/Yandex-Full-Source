package scenario

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var createScenarioProcessorName = "create_scenario_processor"

type CreateScenarioProcessor struct {
	logger         log.Logger
	supController  sup.IController
	linksGenerator sup.AppLinksGenerator
}

func NewCreateScenarioProcessor(
	logger log.Logger,
	supController sup.IController,
	linksGenerator sup.AppLinksGenerator,
) *CreateScenarioProcessor {
	return &CreateScenarioProcessor{
		logger,
		supController,
		linksGenerator,
	}
}

func (p *CreateScenarioProcessor) Name() string {
	return createScenarioProcessorName
}

func (p *CreateScenarioProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.CreateScenarioFrameName,
		},
	}
}

func (p *CreateScenarioProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *CreateScenarioProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	return sdk.RunApplyResponse(new(CreateScenarioApplyArguments))
}

func (p *CreateScenarioProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(CreateScenarioApplyArguments))
}

func (p *CreateScenarioProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	return p.apply(sdk.NewApplyContext(ctx, p.logger, applyRequest, user))
}

func (p *CreateScenarioProcessor) apply(ctx sdk.ApplyContext) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments CreateScenarioApplyArguments
	if err := sdk.UnmarshalApplyArguments(ctx.Arguments(), &applyArguments); err != nil {
		return nil, err
	}
	ctx.Logger().Info("got apply with arguments", log.Any("args", applyArguments))

	clientInfo := ctx.ClientInfo()
	createScenarioPageLink := p.linksGenerator.BuildCreateScenarioPageLink()

	if clientInfo.IsSearchApp() || clientInfo.IsStandalone() {
		directive := libmegamind.OpenURIDirective(createScenarioPageLink)
		return sdk.ApplyResponse(ctx).WithNLG(nlg.ScenarioCreateForSearchApp).WithDirectives(directive).Build()
	}

	user, _ := ctx.User()
	pushTexts := []string{
		"Давайте сделаем сценарий!",
		"Нажмите, чтобы создать сценарий",
		"Пойдемте делать сценарий",
	}
	pushInfo := sup.PushInfo{
		ID:               sup.CreateScenarioPushID,
		Text:             random.Choice(pushTexts),
		Link:             createScenarioPageLink,
		ThrottlePolicyID: sup.CreateScenarioThrottlePolicy,
	}

	if err := p.supController.SendPushToUser(ctx.Context(), user, pushInfo); err != nil {
		ctx.Logger().Errorf("send push to sup failed: %v", err)
		return nil, xerrors.Errorf("failed to send push to sup: %w", err)
	}
	return sdk.ApplyResponse(ctx).WithNLG(nlg.ScenarioCreateForSpeaker).Build()
}
