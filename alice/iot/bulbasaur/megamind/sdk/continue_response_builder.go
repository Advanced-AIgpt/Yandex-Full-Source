package sdk

import (
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type NewContinueResponseBuilder interface {
	WithAnalyticsInfo(analyticsInfo AnalyticsInfoBuilder) NewContinueResponseBuilder
	Build() (*scenarios.TScenarioContinueResponse, error)
}

func ContinueResponse(continueContext ContinueContext) NewContinueResponseBuilder {
	return &continueResponseBuilder{
		ctx:           continueContext,
		analyticsInfo: AnalyticsInfo(),
	}
}

type continueResponseBuilder struct {
	ctx ContinueContext

	analyticsInfo AnalyticsInfoBuilder
}

func (c *continueResponseBuilder) WithAnalyticsInfo(analyticsInfo AnalyticsInfoBuilder) NewContinueResponseBuilder {
	if analyticsInfo != nil {
		c.analyticsInfo = analyticsInfo
	}
	return c
}

func (c *continueResponseBuilder) Build() (*scenarios.TScenarioContinueResponse, error) {
	builtAnalyticsInfo, err := c.analyticsInfo.Build()
	if err != nil {
		return nil, xerrors.Errorf("failed to build analytics info: %w", err)
	}

	responseBody := &scenarios.TScenarioResponseBody{
		AnalyticsInfo: builtAnalyticsInfo,
	}

	return &scenarios.TScenarioContinueResponse{
		Version: buildinfo.Info.SVNRevision,
		Response: &scenarios.TScenarioContinueResponse_ResponseBody{
			ResponseBody: responseBody,
		},
	}, nil
}
