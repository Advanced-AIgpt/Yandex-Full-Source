package apps

import (
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type Commit struct {
}

func (*Commit) isApp() {}

func (*Commit) OnRunRequest(*scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	return &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_CommitCandidate{
			CommitCandidate: &scenarios.TScenarioRunResponse_TCommitCandidate{
				ResponseBody: &scenarios.TScenarioResponseBody{
					Response: &scenarios.TScenarioResponseBody_Layout{
						Layout: &scenarios.TLayout{
							Cards: []*scenarios.TLayout_TCard{
								{
									Card: &scenarios.TLayout_TCard_Text{
										Text: "From DialogId",
									},
								},
							},
						},
					},
				},
			},
		},
	}, nil
}

func (*Commit) OnCommitRequest(*scenarios.TScenarioApplyRequest) (*scenarios.TScenarioCommitResponse, error) {
	return nil, nil
}
