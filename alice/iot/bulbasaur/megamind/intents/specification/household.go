package specification

import (
	"context"
	"fmt"

	"google.golang.org/protobuf/types/known/structpb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	processorName               = "household_specification_processor"
	householdIDPayloadFieldName = "household_id"
)

var SpecifiedHouseholdCallbackName libmegamind.CallbackName = "specified_household"

type HouseholdSpecificationCallback struct {
	HouseholdID string `json:"household_id"`
}

func (h *HouseholdSpecificationCallback) Name() libmegamind.CallbackName {
	return SpecifiedHouseholdCallbackName
}

// HouseholdSpecificationProcessor is an all new and fancy frame-router-compatible processor for household specification
type HouseholdSpecificationProcessor struct {
	logger      log.Logger
	FrameRouter *megamind.FrameRouter
}

func (h *HouseholdSpecificationProcessor) Name() string {
	return processorName
}

func (h *HouseholdSpecificationProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedCallbacks: []libmegamind.CallbackName{
			SpecifiedHouseholdCallbackName,
		},
	}
}

func (h *HouseholdSpecificationProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return h.CoolerRun(sdk.NewRunContext(ctx, h.logger, runRequest, user), sdk.InputFrames(frame))
}

func (h *HouseholdSpecificationProcessor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info("household specification processor is running")
	householdCallback := HouseholdSpecificationCallback{}
	if err := sdk.UnmarshalCallback(input, &householdCallback); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal callback: %w", err)
	}

	householdID := householdCallback.HouseholdID
	state := runContext.Request().GetBaseRequest().GetState()

	var specifyRequestState common.SpecifyRequestState
	if err := specifyRequestState.FromBaseRequestState(state); err != nil {
		return nil, xerrors.Errorf("failed to get specifyRequestState: %w", err)
	}

	return h.FrameRouter.RunWithSpecifiedSlotsV2(
		runContext,
		libmegamind.NewSemanticFrames(specifyRequestState.SemanticFrames),
		&frames.HouseholdSlot{HouseholdID: householdID},
	)
}

func NewHouseholdSpecificationProcessor(logger log.Logger, frameRouter *megamind.FrameRouter) *HouseholdSpecificationProcessor {
	return &HouseholdSpecificationProcessor{
		logger:      logger,
		FrameRouter: frameRouter,
	}
}

func HouseholdSpecificationCallbacks(households []model.Household, inf inflector.IInflector) []libmegamind.CallbackFrameAction {
	result := make([]libmegamind.CallbackFrameAction, 0, len(households))
	for _, household := range households {
		nameInflection := inflector.TryInflect(inf, household.Name, inflector.GrammaticalCases)
		result = append(result, libmegamind.CallbackFrameAction{
			FrameSlug:    fmt.Sprintf("%s_%s", SpecifiedHouseholdCallbackName, household.ID),
			FrameName:    fmt.Sprintf("%s_%s", SpecifiedHouseholdCallbackName, household.ID),
			CallbackName: SpecifiedHouseholdCallbackName,
			Phrases: []string{
				nameInflection.Im,
				fmt.Sprintf("В %s", nameInflection.Pr),
				fmt.Sprintf("На %s", nameInflection.Pr),
				nameInflection.Vin,
				fmt.Sprintf("В %s", nameInflection.Im),
				fmt.Sprintf("На %s", nameInflection.Im),
			},
			CallbackPayload: &structpb.Struct{
				Fields: map[string]*structpb.Value{
					householdIDPayloadFieldName: {
						Kind: &structpb.Value_StringValue{
							StringValue: household.ID,
						},
					},
				},
			},
		})
	}
	return result
}
