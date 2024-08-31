package sdk

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func RunResponseBuilder(runContext RunContext, args UniversalApplyArguments) (*scenarios.TScenarioRunResponse, error) {
	protoArgs, err := MarshalApplyArguments(args)
	if err != nil {
		return nil, err
	}
	return libmegamind.NewRunResponse("iot", "iot").WithApplyArguments(protoArgs).Build(), nil
}

func CommonErrorRunResponseBuilder(runContext RunContext, err error) *scenarios.TScenarioRunResponse {
	return CustomErrorRunResponseBuilder(runContext, nlg.CommonError, err)
}

func CustomErrorRunResponseBuilder(runContext RunContext, nlg libnlg.NLG, err error) *scenarios.TScenarioRunResponse {
	runContext.Logger().Errorf("failed to perform run: %v", err)
	asset := nlg.RandomAsset(runContext.Context())
	runContext.Logger().Infof("NlgRunResponse: `%s`", asset.Text())
	return libmegamind.NewRunResponse("iot", "iot").WithLayout(asset.Text(), asset.Speech()).Build()
}

func ApplyResponseBuilder(applyContext ApplyContext, nlg libnlg.NLG, directives ...*scenarios.TDirective) *scenarios.TScenarioApplyResponse {
	response := libmegamind.NewApplyResponse("iot", "iot")
	if nlg != nil {
		asset := nlg.RandomAsset(applyContext.Context())
		applyContext.Logger().Infof("nlg: text %q, speech %q, isTextOnly: %v", asset.Text(), asset.Speech(), asset.IsTextOnly())
		response.WithLayout(asset.Text(), asset.Speech())
	} else {
		applyContext.Logger().Infof("nlg: silent")
	}
	if len(directives) > 0 {
		response.WithDirectives(directives...)
	}
	return response.Build()
}

func CommonErrorApplyResponseBuilder(applyContext ApplyContext, err error) *scenarios.TScenarioApplyResponse {
	applyContext.Logger().Errorf("failed to perform apply: %v", err)
	return ApplyResponseBuilder(applyContext, nlg.CommonError)
}
