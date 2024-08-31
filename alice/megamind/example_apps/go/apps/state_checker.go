package apps

import (
	"strconv"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/any"
	"github.com/golang/protobuf/ptypes/struct"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type StateChecker struct {
}

func (*StateChecker) isApp() {}

func (stateChecker *StateChecker) OnRunRequest(runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	data := &structpb.Struct{}
	if err := proto.Unmarshal(runRequest.BaseRequest.State.Value, data); err != nil {
		data.Fields["id"] = &structpb.Value{Kind: &structpb.Value_NumberValue{NumberValue: 0}}
	}
	number := data.GetFields()["id"].GetNumberValue()
	data.Fields = map[string]*structpb.Value{
		"id": {
			Kind: &structpb.Value_NumberValue{
				NumberValue: number + 1,
			},
		},
	}
	state := &any.Any{}
	binary, err := proto.Marshal(data)
	if err != nil {
		return nil, err
	}
	state.Value = binary

	divBody := `{
    "background": [
        {
            "color": "#FFFFFF",
            "type": "div-solid-background"
        }
    ],
    "states": [
        {
            "blocks": [
                {
                    "title": "Проверка сессии",
                    "title_style": "card_header",
                    "text": "#` + strconv.Itoa(int(number)) + `",
                    "text_max_lines": 3,
                    "type": "div-universal-block",
                    "text_style": "text_l"
                }
            ],
            "state_id": 1
        }
    ]
}`
	divCardStruct := &structpb.Struct{}
	err = jsonpb.UnmarshalString(divBody, divCardStruct)
	if err != nil {
		return nil, err
	}
	dialogID := "abacasbasd"
	_ = dialogID
	response := &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: &scenarios.TLayout{
						Cards: []*scenarios.TLayout_TCard{
							{
								Card: &scenarios.TLayout_TCard_DivCard{
									DivCard: divCardStruct,
								},
							},
							//{
							//	Card: &scenarios.TLayout_TCard_Text{
							//		Text:strconv.Itoa(int(number)),
							//	},
							//},
							{
								Card: &scenarios.TLayout_TCard_TextWithButtons{
									TextWithButtons: &scenarios.TLayout_TTextWithButtons{
										Text: "Кнопки управления",
										Buttons: []*scenarios.TLayout_TButton{
											{
												Title: "OpenDialog",
												//Directives: []*scenarios.TDirective{
												//	{
												//		Directive: &scenarios.TDirective_OpenDialogDirective{
												//			OpenDialogDirective: &scenarios.TOpenDialogDirective{
												//				Name:     "Name",
												//				DialogId: dialogID,
												//				Directives: []*scenarios.TDirective{
												//					{
												//						Directive: &scenarios.TDirective_EndDialogSessionDirective{
												//							EndDialogSessionDirective: &scenarios.TEndDialogSessionDirective{
												//								DialogId: "go_app:" + dialogID,
												//							},
												//						},
												//					},
												//					{
												//						Directive: &scenarios.TDirective_UpdateDialogInfoDirective{
												//							UpdateDialogInfoDirective: &scenarios.TUpdateDialogInfoDirective{
												//								Name: "Dialog Name",
												//								Style: &scenarios.TUpdateDialogInfoDirective_TStyle{
												//									SuggestBorderColor:    "#6839cf",
												//									UserBubbleFillColor:   "#eeeeee",
												//									SuggestTextColor:      "#6839cf",
												//									SuggestFillColor:      "#ffffff",
												//									UserBubbleTextColor:   "#ffffff",
												//									SkillActionsTextColor: "#6839cf",
												//									SkillBubbleFillColor:  "#f0f0f5",
												//									SkillBubbleTextColor:  "#cc000000",
												//									OknyxLogo:             "alice",
												//									OknyxErrorColors: []string{
												//										"#ff4050",
												//										"#ff4050",
												//									},
												//									OknyxNormalColors: []string{
												//										"#c926ff",
												//										"#4a26ff",
												//									},
												//								},
												//								Title:    "Dialog Title",
												//								URL:      "https://ya.ru",
												//								ImageUrl: "https://avatars.mdst.yandex.net/get-dialogs/758954/a22fd3e7f536e9816a56/mobile-logo-x2",
												//								MenuItems: []*scenarios.TUpdateDialogInfoDirective_TMenuItem{
												//									{
												//										URL:   "https://ya.ru",
												//										Title: "ButtonTitle",
												//									},
												//								},
												//							},
												//						},
												//					},
												//					{
												//						Directive: &scenarios.TDirective_CallbackDirective{
												//							CallbackDirective: &scenarios.TCallbackDirective{
												//								Name:         "new_dialog_session",
												//								IgnoreAnswer: false,
												//								Payload:      nil,
												//							},
												//						},
												//					},
												//				},
												//			},
												//		},
												//	},
												//},
											},
										},
									},
								},
							},
						},
					},
				},
				State: state,
			},
		},
	}
	return response, nil
}
