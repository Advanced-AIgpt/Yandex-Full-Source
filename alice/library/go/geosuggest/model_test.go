package geosuggest

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestGeosuggestUnmarshalling(t *testing.T) {
	geosuggestAnswer := `{
    "part": "Подольск Давы",
    "suggest_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
    "results": [
        {
            "type": "toponym",
            "pos": "37.559948,55.403053",
            "log_id": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 0,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова, 16",
                    "source_id": "56670366",
                    "title": "улица Давыдова, 16"
                }
            },
            "personalization_info": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 0,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова, 16",
                    "source_id": "56670366",
                    "title": "улица Давыдова, 16"
                }
            },
            "title": {
                "text": "улица Давыдова, 16",
                "hl": [
                    [
                        6,
                        10
                    ]
                ]
            },
            "subtitle": {
                "text": "Подольск, Московская область, Россия",
                "hl": [
                    [
                        0,
                        8
                    ]
                ]
            },
            "text": "Россия, Московская область, Подольск, улица Давыдова, 16 ",
            "tags": [
                "house"
            ],
            "action": "search",
            "uri": "ymapsbm1://geo?ll=37.559948%2C55.403053&spn=0.001000%2C0.001000&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%BE%D0%B2%D1%81%D0%BA%D0%B0%D1%8F%20%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C%2C%20%D0%9F%D0%BE%D0%B4%D0%BE%D0%BB%D1%8C%D1%81%D0%BA%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%94%D0%B0%D0%B2%D1%8B%D0%B4%D0%BE%D0%B2%D0%B0%2C%2016%20",
            "distance": {
                "value": 7026169.805,
                "text": "7026.17 км"
            }
        },
        {
            "type": "toponym",
            "pos": "37.558582,55.403271",
            "log_id": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 1,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова",
                    "source_id": "10048652",
                    "title": "улица Давыдова"
                }
            },
            "personalization_info": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 1,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова",
                    "source_id": "10048652",
                    "title": "улица Давыдова"
                }
            },
            "title": {
                "text": "улица Давыдова",
                "hl": [
                    [
                        6,
                        10
                    ]
                ]
            },
            "subtitle": {
                "text": "Подольск, Московская область, Россия",
                "hl": [
                    [
                        0,
                        8
                    ]
                ]
            },
            "text": "Россия, Московская область, Подольск, улица Давыдова ",
            "tags": [
                "street"
            ],
            "action": "substitute",
            "uri": "ymapsbm1://geo?ll=37.558582%2C55.403271&spn=0.006863%2C0.001000&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%BE%D0%B2%D1%81%D0%BA%D0%B0%D1%8F%20%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C%2C%20%D0%9F%D0%BE%D0%B4%D0%BE%D0%BB%D1%8C%D1%81%D0%BA%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%94%D0%B0%D0%B2%D1%8B%D0%B4%D0%BE%D0%B2%D0%B0%20",
            "distance": {
                "value": 7026127.772,
                "text": "7026.13 км"
            }
        },
        {
            "type": "toponym",
            "pos": "37.397488,55.460453",
            "log_id": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 2,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова",
                    "source_id": "1661757561",
                    "title": "улица Давыдова"
                }
            },
            "personalization_info": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 2,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова",
                    "source_id": "1661757561",
                    "title": "улица Давыдова"
                }
            },
            "title": {
                "text": "улица Давыдова",
                "hl": [
                    [
                        6,
                        10
                    ]
                ]
            },
            "subtitle": {
                "text": "посёлок Поливаново, городской округ Подольск, Московская область, Россия",
                "hl": [
                    [
                        36,
                        44
                    ]
                ]
            },
            "text": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова ",
            "tags": [
                "street"
            ],
            "action": "substitute",
            "uri": "ymapsbm1://geo?ll=37.397488%2C55.460453&spn=0.001000%2C0.002144&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%BE%D0%B2%D1%81%D0%BA%D0%B0%D1%8F%20%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C%2C%20%D0%B3%D0%BE%D1%80%D0%BE%D0%B4%D1%81%D0%BA%D0%BE%D0%B9%20%D0%BE%D0%BA%D1%80%D1%83%D0%B3%20%D0%9F%D0%BE%D0%B4%D0%BE%D0%BB%D1%8C%D1%81%D0%BA%2C%20%D0%BF%D0%BE%D1%81%D1%91%D0%BB%D0%BE%D0%BA%20%D0%9F%D0%BE%D0%BB%D0%B8%D0%B2%D0%B0%D0%BD%D0%BE%D0%B2%D0%BE%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%94%D0%B0%D0%B2%D1%8B%D0%B4%D0%BE%D0%B2%D0%B0%20",
            "distance": {
                "value": 7023757.693,
                "text": "7023.76 км"
            }
        },
        {
            "type": "toponym",
            "pos": "37.398037,55.460644",
            "log_id": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 3,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова, 8",
                    "source_id": "3422645545",
                    "title": "улица Давыдова, 8"
                }
            },
            "personalization_info": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 3,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова, 8",
                    "source_id": "3422645545",
                    "title": "улица Давыдова, 8"
                }
            },
            "title": {
                "text": "улица Давыдова, 8",
                "hl": [
                    [
                        6,
                        10
                    ]
                ]
            },
            "subtitle": {
                "text": "посёлок Поливаново, городской округ Подольск, Московская область, Россия",
                "hl": [
                    [
                        36,
                        44
                    ]
                ]
            },
            "text": "Россия, Московская область, городской округ Подольск, посёлок Поливаново, улица Давыдова, 8 ",
            "tags": [
                "house"
            ],
            "action": "search",
            "uri": "ymapsbm1://geo?ll=37.398037%2C55.460644&spn=0.001000%2C0.001000&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%BE%D0%B2%D1%81%D0%BA%D0%B0%D1%8F%20%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C%2C%20%D0%B3%D0%BE%D1%80%D0%BE%D0%B4%D1%81%D0%BA%D0%BE%D0%B9%20%D0%BE%D0%BA%D1%80%D1%83%D0%B3%20%D0%9F%D0%BE%D0%B4%D0%BE%D0%BB%D1%8C%D1%81%D0%BA%2C%20%D0%BF%D0%BE%D1%81%D1%91%D0%BB%D0%BE%D0%BA%20%D0%9F%D0%BE%D0%BB%D0%B8%D0%B2%D0%B0%D0%BD%D0%BE%D0%B2%D0%BE%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%94%D0%B0%D0%B2%D1%8B%D0%B4%D0%BE%D0%B2%D0%B0%2C%208%20",
            "distance": {
                "value": 7023797.149,
                "text": "7023.80 км"
            }
        },
        {
            "type": "toponym",
            "pos": "37.560890,55.404270",
            "log_id": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 4,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова, 5",
                    "source_id": "1642751971",
                    "title": "улица Давыдова, 5"
                }
            },
            "personalization_info": {
                "server_reqid": "1614788189375165-545108159-kqppo7ggpyc3fgep",
                "pos": 4,
                "type": "toponym",
                "where": {
                    "name": "Россия, Московская область, Подольск, улица Давыдова, 5",
                    "source_id": "1642751971",
                    "title": "улица Давыдова, 5"
                }
            },
            "title": {
                "text": "улица Давыдова, 5",
                "hl": [
                    [
                        6,
                        10
                    ]
                ]
            },
            "subtitle": {
                "text": "Подольск, Московская область, Россия",
                "hl": [
                    [
                        0,
                        8
                    ]
                ]
            },
            "text": "Россия, Московская область, Подольск, улица Давыдова, 5 ",
            "tags": [
                "house"
            ],
            "action": "search",
            "uri": "ymapsbm1://geo?ll=37.560890%2C55.404270&spn=0.001000%2C0.001000&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%BE%D0%B2%D1%81%D0%BA%D0%B0%D1%8F%20%D0%BE%D0%B1%D0%BB%D0%B0%D1%81%D1%82%D1%8C%2C%20%D0%9F%D0%BE%D0%B4%D0%BE%D0%BB%D1%8C%D1%81%D0%BA%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%94%D0%B0%D0%B2%D1%8B%D0%B4%D0%BE%D0%B2%D0%B0%2C%205%20",
            "distance": {
                "value": 7026309.941,
                "text": "7026.31 км"
            }
        }
    ]
}`
	var response GeosuggestFromAddressResponse
	err := json.Unmarshal([]byte(geosuggestAnswer), &response)
	assert.NoError(t, err)
	assert.Equal(t, 5, len(response.Results))
	assert.Equal(t, "Россия, Московская область, Подольск, улица Давыдова, 16", response.Results[0].Address())
	coordinates, err := response.Results[0].Coordinates()
	assert.NoError(t, err)
	assert.Equal(t, 37.559948, coordinates.Longitude)
	assert.Equal(t, 55.403053, coordinates.Latitude)
	assert.Equal(t, "Подольск Давы", response.Part)
	assert.Equal(t, "Улица Давыдова, 16", response.Results[0].ShortAddress())
}

func TestGeosuggestContainsExactAddress(t *testing.T) {
	response := GeosuggestFromAddressResponse{
		Part: "Моя хата с краю ничего не знаю",
		Results: []Geosuggest{
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Моя хата с краю ничего не знаю",
				Title:          GeosuggestTitle{RawShortAddress: "Моя хата с краю ничего не знаю, но короткий"},
			},
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Москва, улица Льва Толстого, 16",
				Title:          GeosuggestTitle{RawShortAddress: "улица Льва Толстого, 16"},
			},
			{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Московская область, Балашиха, квартал Абрамцево, 53",
				Title:          GeosuggestTitle{RawShortAddress: "Квартал Абрамцево, 53"},
			},
		},
	}
	type testCase struct {
		address              string
		additionalComponents []string
		expected             Geosuggest
		hasAddress           bool
	}
	testCases := []testCase{
		{
			address:    "короткая хата",
			hasAddress: false,
			expected:   Geosuggest{},
		},
		{
			address:    "Моя хата с краю ничего не знаю",
			hasAddress: true,
			expected: Geosuggest{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Моя хата с краю ничего не знаю",
				Title:          GeosuggestTitle{RawShortAddress: "Моя хата с краю ничего не знаю, но короткий"},
			},
		},
		{
			address:    "Улица Льва Толстого, 16, Москва, Россия",
			hasAddress: true,
			expected: Geosuggest{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Москва, улица Льва Толстого, 16",
				Title:          GeosuggestTitle{RawShortAddress: "улица Льва Толстого, 16"},
			},
		},
		{
			address:              "Улица Льва Толстого, 16, Москва, Россия",
			hasAddress:           true,
			additionalComponents: []string{"россия"},
			expected: Geosuggest{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Москва, улица Льва Толстого, 16",
				Title:          GeosuggestTitle{RawShortAddress: "улица Льва Толстого, 16"},
			},
		},
		{
			address:              "Московская область, Балашиха, квартал Абрамцево, 53",
			hasAddress:           true,
			additionalComponents: []string{"россия"},
			expected: Geosuggest{
				RawCoordinates: "55.555555,66.666666",
				RawAddress:     "Россия, Московская область, Балашиха, квартал Абрамцево, 53",
				Title:          GeosuggestTitle{RawShortAddress: "Квартал Абрамцево, 53"},
			},
		},
	}
	for _, tc := range testCases {
		gs, hasAddress := response.HasValidAddress(tc.address, tc.additionalComponents)
		assert.Equal(t, tc.hasAddress, hasAddress)
		if !tc.hasAddress {
			continue
		}
		assert.Equal(t, tc.expected, gs)
	}

}
