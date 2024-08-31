package apps

import (
	"fmt"
	"time"

	"github.com/golang/protobuf/ptypes"

	"a.yandex-team.ru/alice/megamind/example_apps/go/apps/proto"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type MementoApp struct {
}

func (*MementoApp) isApp() {}

func (mementoApp *MementoApp) OnRunRequest(runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	mementoDataPacked := runRequest.GetBaseRequest().GetMemento().GetScenarioData()
	var data apps_proto.TMementoPayload
	err := ptypes.UnmarshalAny(mementoDataPacked, &data)
	if err != nil {
		data = apps_proto.TMementoPayload{
			Counter: 0,
		}
	}
	data = apps_proto.TMementoPayload{
		RequestId: runRequest.GetBaseRequest().GetRequestId(),
		Timestamp: time.Now().Unix(),
		Counter:   data.GetCounter() + 1,
	}
	mementoData, err := ptypes.MarshalAny(&data)
	if err != nil {
		return nil, err
	}
	return &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: &scenarios.TLayout{
						Cards: []*scenarios.TLayout_TCard{
							{
								Card: &scenarios.TLayout_TCard_Text{
									Text: fmt.Sprintf("Saving to memento(%d)", data.GetCounter()),
								},
							},
						},
					},
				},
				ServerDirectives: []*scenarios.TServerDirective{
					{
						Directive: &scenarios.TServerDirective_MementoChangeUserObjectsDirective{
							MementoChangeUserObjectsDirective: &scenarios.TMementoChangeUserObjectsDirective{
								UserObjects: &scenarios.TMementoChangeUserObjectsDirective_TReqChangeUserObjects{
									ScenarioData: mementoData,
								},
							},
						},
					},
				},
			},
		},
	}, nil
}
