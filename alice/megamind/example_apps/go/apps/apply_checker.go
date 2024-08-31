package apps

import (
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"fmt"
	"github.com/golang/protobuf/proto"
	_any "github.com/golang/protobuf/ptypes/any"
	_struct "github.com/golang/protobuf/ptypes/struct"
	"time"
)

type ApplyChecker struct {
}

func (*ApplyChecker) isApp() {}

func (echoApply *ApplyChecker) OnRunRequest(runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	data := &_struct.Struct{Fields: map[string]*_struct.Value{
		"request_id": {
			Kind: &_struct.Value_StringValue{
				StringValue: runRequest.GetBaseRequest().GetRequestId(),
			},
		},
		"time": {
			Kind: &_struct.Value_NumberValue{
				NumberValue: float64(time.Now().Unix()),
			},
		},
	}}
	bytes, err := proto.Marshal(data)
	if err != nil {
		return nil, err
	}
	applyArguments := &scenarios.TScenarioRunResponse_ApplyArguments{
		ApplyArguments: &_any.Any{
			Value: bytes,
		},
	}
	return &scenarios.TScenarioRunResponse{
		Response: applyArguments,
	}, nil
}

func (echoApply *ApplyChecker) OnApplyRequest(request *scenarios.TScenarioApplyRequest) (*scenarios.TScenarioApplyResponse, error) {
	state := &_struct.Struct{}
	err := proto.Unmarshal(request.Arguments.Value, state)
	if err != nil {
		return nil, err
	}
	message := fmt.Sprintf(`Apply response
RunRequestId: %s
RunTime: %f
ApplyRequestId: %s
ApplyTime: %d
`, state.Fields["request_id"].GetStringValue(), state.Fields["time"].GetNumberValue(), request.BaseRequest.RequestId, time.Now().Unix())
	return &scenarios.TScenarioApplyResponse{
		Response: &scenarios.TScenarioApplyResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: &scenarios.TLayout{
						Cards: []*scenarios.TLayout_TCard{
							{
								Card: &scenarios.TLayout_TCard_Text{
									Text: message,
								},
							},
						},
						OutputSpeech: "Прив+ет из г+олэнга!",
					},
				},
				ExpectsRequest: true,
			},
		},
	}, nil
}
