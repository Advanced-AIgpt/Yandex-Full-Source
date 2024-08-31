package apps

import (
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/ptypes/struct"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type CardsProvider struct {
}

func (*CardsProvider) isApp() {}

func (cardsProvider *CardsProvider) OnRunRequest(runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	if runRequest.BaseRequest.DialogId != "" {
		return &scenarios.TScenarioRunResponse{
			Response: &scenarios.TScenarioRunResponse_ResponseBody{
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
		}, nil
	}

	dialogID := "deadbeef-dialog_id_1234"
	_ = dialogID
	textCard := &scenarios.TLayout_TCard{
		Card: &scenarios.TLayout_TCard_Text{
			Text: "Hi! I'm text card!",
		},
	}
	textWithButtonsCard := &scenarios.TLayout_TCard{
		Card: &scenarios.TLayout_TCard_TextWithButtons{
			TextWithButtons: &scenarios.TLayout_TTextWithButtons{
				Text: "Hi! I'm text with buttons!",
				Buttons: []*scenarios.TLayout_TButton{
					{
						Title: "OpenUri",
						//Directives: []*scenarios.TDirective{
						//	{
						//		Directive: &scenarios.TDirective_OpenUriDirective{
						//			OpenUriDirective: &scenarios.TOpenUriDirective{
						//				Name: "URI",
						//				URI:  "https://wiki.yandex-team.ru/alice/megamind/protocolscenarios/proto/",
						//			},
						//		},
						//	},
						//},
					},
					{
						Title: "TypeText",
						//Directives: []*scenarios.TDirective{
						//	{
						//		Directive: &scenarios.TDirective_TypeTextDirective{
						//			TypeTextDirective: &scenarios.TTypeTextDirective{
						//				Name: "Hello",
						//				Text: "I'm text from type text directive",
						//			},
						//		},
						//	},
						//},
					},
					{
						Title: "TypeTextSilent",
						//Directives: []*scenarios.TDirective{
						//	{
						//		Directive: &scenarios.TDirective_TypeTextSilentDirective{
						//			TypeTextSilentDirective: &scenarios.TTypeTextSilentDirective{
						//				Name: "HelloSilent",
						//				Text: "I'm text from type text silent directive",
						//			},
						//		},
						//	},
						//},
					},
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
						//								DialogId: dialogID,
						//							},
						//						},
						//					},
						//					{
						//						Directive: &scenarios.TDirective_UpdateDialogInfoDirective{
						//							UpdateDialogInfoDirective: &scenarios.TUpdateDialogInfoDirective{
						//								Name: "Dialog Name",
						//								Style: &scenarios.TUpdateDialogInfoDirective_TStyle{
						//									SuggestBorderColor:    "#6839cf",
						//									UserBubbleFillColor:   "#ffffff",
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
	}
	divCardJSON := `{
    "background": [
        {
            "color": "#FFFFFF",
            "type": "div-solid-background"
        }
    ],
    "states": [
        {
            "action": {
                "log_id": "whole_card",
                "url": "yellowskin://?primary_color=%23ffffff&secondary_color=%23000000&url=https%3A//yandex.ru/pogoda/anosino%3Fappsearch_header%3D1%26appsearch_ys%3D2%23d_11"
            },
            "blocks": [
                {
                    "size": "xs",
                    "type": "div-separator-block"
                },
                {
                    "title": "Привет, я дивная карточка!",
                    "type": "div-universal-block"
                },
                {
                    "columns": [
                        {},
                        {
                            "left_padding": "zero"
                        }
                    ],
                    "rows": [
                        {
                            "cells": [
                                {
                                    "horizontal_alignment": "left",
                                    "text": " +25 °",
                                    "text_style": "numbers_l"
                                },
                                {
                                    "horizontal_alignment": "left",
                                    "image": {
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/787408/weather_60x60_b5e1fe5a03fe686298cd7f8a3e87b0c1752f2daf6014369fa47ba3c7e901c1a4.png/orig",
                                        "ratio": 1,
                                        "type": "div-image-element"
                                    },
                                    "image_size": "xxl"
                                }
                            ],
                            "type": "row_element"
                        }
                    ],
                    "type": "div-table-block"
                },
                {
                    "text": "облачно с прояснениями",
                    "type": "div-universal-block"
                },
                {
                    "size": "xxs",
                    "type": "div-separator-block"
                },
                {
                    "has_delimiter": 1,
                    "size": "xxs",
                    "type": "div-separator-block"
                },
                {
                    "size": "s",
                    "type": "div-separator-block"
                },
                {
                    "columns": [
                        {
                            "right_padding": "xxl"
                        },
                        {
                            "right_padding": "xxl"
                        },
                        {
                            "weight": 0
                        }
                    ],
                    "rows": [
                        {
                            "cells": [
                                {
                                    "text": "Утро ",
                                    "text_style": "text_s"
                                },
                                {
                                    "text": "День ",
                                    "text_style": "text_s"
                                },
                                {
                                    "text": "Вечер ",
                                    "text_style": "text_s"
                                }
                            ],
                            "type": "row_element"
                        },
                        {
                            "cells": [
                                {
                                    "image": {
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/397492/weather_30x30_920025169ce1129c6fdf5ad7196685bccbd0c51285a7e29ce2cc7a4e77764f92.png/orig",
                                        "ratio": 1,
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs",
                                    "text": " +16 "
                                },
                                {
                                    "image": {
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/397492/weather_30x30_920025169ce1129c6fdf5ad7196685bccbd0c51285a7e29ce2cc7a4e77764f92.png/orig",
                                        "ratio": 1,
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs",
                                    "text": " +24 "
                                },
                                {
                                    "image": {
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/397492/weather_30x30_1c38dbb75a0362b627f13f95b80f7b6768fc3292fc3751f6c78a2b0539cf44d1.png/orig",
                                        "ratio": 1,
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs",
                                    "text": " +19 "
                                }
                            ],
                            "type": "row_element"
                        }
                    ],
                    "type": "div-table-block"
                },
                {
                    "size": "s",
                    "type": "div-separator-block"
                }
            ],
            "state_id": 1
        }
    ]
}
`

	divCardStruct := &structpb.Struct{}
	err := jsonpb.UnmarshalString(divCardJSON, divCardStruct)
	if err != nil {
		return nil, err
	}
	divCard := &scenarios.TLayout_TCard{
		Card: &scenarios.TLayout_TCard_DivCard{
			DivCard: divCardStruct,
		},
	}

	div2CardJSON := `{
		"log_id": "gif_card",
		"states": [
		  {
			"div": {
			  "gif_click_url": "yandex.ru",
			  "gif_description": "Источник: yandex.ru",
			  "gif_image_url": "https://cvlab.s3.yandex.net/emoji-gifs/3BM.gif",
			  "type": "gif_card"
			},
			"state_id": 1
		  }
		]
	  }`

	div2CardStruct := &structpb.Struct{}
	_ = jsonpb.UnmarshalString(div2CardJSON, div2CardStruct)

	div2Card := &scenarios.TLayout_TCard{
		Card: &scenarios.TLayout_TCard_Div2CardExtended{
			Div2CardExtended: &scenarios.TLayout_TCard_TDiv2Card{
				Body:        div2CardStruct,
				HideBorders: true,
			},
		},
	}

	rawTemplates := `{
		"gif_card": {
			"type": "container",
			"direction": "vertical",
			"width": {
				"type": "fixed",
				"value": 280
			},
			"height": {
				"type": "fixed",
				"value": 280
			},
			"items": [
				{
					"type": "gif",
					"$gif_url": "gif_image_url",
					"action": {
						"url": "https://ya.ru",
						"log_id": "test_log_id"
					},
					"width": {
						"type": "fixed",
						"value": 280
					},
					"height": {
						"type": "fixed",
						"value": 280
					}
				},
				{
					"$text": "gif_description",
					"font_size": 14,
					"paddings": {
						"bottom": 8,
						"left": 12,
						"right": 12,
						"top": 8
					},
					"text_color": "#808080",
					"type": "text"
				}
			]
		}
	}`

	templates := &structpb.Struct{}
	_ = jsonpb.UnmarshalString(rawTemplates, templates)

	return &scenarios.TScenarioRunResponse{
		Features: &scenarios.TScenarioRunResponse_TFeatures{
			IsIrrelevant: false,
		},
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: &scenarios.TLayout{
						Cards: []*scenarios.TLayout_TCard{
							textCard,
							textWithButtonsCard,
							divCard,
							div2Card,
						},
						Suggests: []*scenarios.TLayout_TButton{
							{
								Title: "I'm suggest",
								//Directives: []*scenarios.TDirective{
								//	{
								//		Directive: &scenarios.TDirective_TypeTextSilentDirective{
								//			TypeTextSilentDirective: &scenarios.TTypeTextSilentDirective{
								//				Name: "Name",
								//				Text: "Text",
								//			},
								//		},
								//	},
								//	{
								//		Directive: &scenarios.TDirective_CallbackDirective{
								//			CallbackDirective: &scenarios.TCallbackDirective{
								//				Name: "callback_directive",
								//			},
								//		},
								//	},
								//},
							},
						},
						Directives: []*scenarios.TDirective{
							{
								Directive: &scenarios.TDirective_TypeTextSilentDirective{
									TypeTextSilentDirective: &scenarios.TTypeTextSilentDirective{
										Name: "Name",
										Text: "Text",
									},
								},
							},
						},
						Div2Templates: templates,
					},
				},
			},
		},
	}, nil
}
