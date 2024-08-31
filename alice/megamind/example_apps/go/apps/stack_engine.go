package apps

import (
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"fmt"
	"github.com/golang/protobuf/proto"
	_any "github.com/golang/protobuf/ptypes/any"
	_struct "github.com/golang/protobuf/ptypes/struct"
	"time"
)

type StackEngine struct{}

func (*StackEngine) isApp() {}

func constructApplyArguments(requestID string) (*scenarios.TScenarioRunResponse_ApplyArguments, error) {
	data := &_struct.Struct{Fields: map[string]*_struct.Value{
		"request_id": {
			Kind: &_struct.Value_StringValue{
				StringValue: requestID,
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
	return &scenarios.TScenarioRunResponse_ApplyArguments{
		ApplyArguments: &_any.Any{
			Value: bytes,
		},
	}, nil
}

func (se *StackEngine) OnRunRequest(runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	if cb, ok := runRequest.Input.Event.(*scenarios.TInput_Callback); ok {
		if cb.Callback.Name == "3" {
			applyArguments, err := constructApplyArguments(runRequest.GetBaseRequest().GetRequestId())
			if err != nil {
				return nil, err
			}
			return &scenarios.TScenarioRunResponse{
				Response: applyArguments,
			}, nil
		}
		return &scenarios.TScenarioRunResponse{
			Response: &scenarios.TScenarioRunResponse_ResponseBody{
				ResponseBody: &scenarios.TScenarioResponseBody{
					Response: &scenarios.TScenarioResponseBody_Layout{
						Layout: &scenarios.TLayout{
							Cards: []*scenarios.TLayout_TCard{
								{
									Card: &scenarios.TLayout_TCard_Text{
										Text: cb.Callback.Name,
									},
								},
							},
						},
					},
				},
			},
		}, nil
	}
	return &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: &scenarios.TLayout{
						Cards: []*scenarios.TLayout_TCard{
							{
								Card: &scenarios.TLayout_TCard_Text{
									Text: "Первое сообщение",
								},
							},
						},
					},
				},
				StackEngine: &scenarios.TStackEngine{
					Actions: []*scenarios.TStackEngineAction{
						{
							Action: &scenarios.TStackEngineAction_NewSession{
								NewSession: &scenarios.TStackEngineAction_TNewSession{},
							},
						},
						{
							Action: &scenarios.TStackEngineAction_ResetAdd{
								ResetAdd: &scenarios.TStackEngineAction_TResetAdd{
									Effects: []*scenarios.TStackEngineEffect{
										{
											Effect: &scenarios.TStackEngineEffect_Callback{
												Callback: &scenarios.TCallbackDirective{
													Name: "4",
												},
											},
										},
										{
											Effect: &scenarios.TStackEngineEffect_Callback{
												Callback: &scenarios.TCallbackDirective{
													Name: "3",
												},
											},
										},
										{
											Effect: &scenarios.TStackEngineEffect_Callback{
												Callback: &scenarios.TCallbackDirective{
													Name: "2",
												},
											},
										},
										{
											Effect: &scenarios.TStackEngineEffect_Callback{
												Callback: &scenarios.TCallbackDirective{
													Name: "1",
												},
											},
										},
									},
									RecoveryAction: &scenarios.TStackEngineAction_TResetAdd_TRecoveryAction{
										Callback: &scenarios.TCallbackDirective{
											Name: "1",
										},
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

func (se *StackEngine) OnApplyRequest(applyRequest *scenarios.TScenarioApplyRequest) (*scenarios.TScenarioApplyResponse, error) {
	state := &_struct.Struct{}
	err := proto.Unmarshal(applyRequest.Arguments.Value, state)
	if err != nil {
		return nil, err
	}
	message := fmt.Sprintf(
		"Apply response\nRunRequestId: %s\nRunTime: %f\nApplyRequestId: %s\nApplyTime: %d",
		state.Fields["request_id"].GetStringValue(),
		state.Fields["time"].GetNumberValue(),
		applyRequest.BaseRequest.RequestId,
		time.Now().Unix(),
	)
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
					},
				},
			},
		},
	}, nil
}
