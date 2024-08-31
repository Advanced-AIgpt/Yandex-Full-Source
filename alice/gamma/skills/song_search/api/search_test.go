package api

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
)

var testLogger = sdkTest.CreateTestLogger()

func TestTrack_ToSearchResult(t *testing.T) {
	tracks := []Track{
		{
			ID:    1,
			Title: "It's going down",
			Artists: []Artist{
				{Name: "Linkin Park"}, {Name: "Wayne Static"},
			},
			Available: true,
			Explicit:  true,
		},
		{
			ID:    2,
			Title: "The Chain",
			Artists: []Artist{
				{Name: "Fleetwood Mac"},
			},
			Available: true,
			Explicit:  false,
			PoetryCite: &PoetryCite{
				Lines: []PoetryLine{
					{Text: "And if, you don't love me now ", Line: 0},
					{Text: "You will never love me again ", Line: 1},
					{Text: "I can still hear you saying ", Line: 2},
					{Text: "You would never break the chain ", Line: 3},
				},
			},
		},
	}

	expectedResults := []SearchResult{
		{
			Title:    "It's going down",
			Artist:   "Linkin Park, Wayne Static",
			Explicit: true,
			URL:      "https://music.yandex.ru/track/1",
		},
		{
			Title:  "The Chain",
			Artist: "Fleetwood Mac",
			Text: "And if, you don't love me now \n" +
				"You will never love me again \n" +
				"I can still hear you saying \n" +
				"You would never break the chain ",
			URL: "https://music.yandex.ru/track/2",
		},
	}

	for i := range tracks {
		assert.Equal(t, expectedResults[i], tracks[i].ToSearchResult(4))
	}
}

func TestMusicApiSearch_newSearchResponse(t *testing.T) {
	responseString := `
{
    "invocationInfo":{
        "hostname":"music-qa-pr-back-5-2.sas.yp-c.yandex.net",
        "req-id":"fad8810de1fabcbe0b315e8814178487",
        "exec-duration-millis":"219"
    },
    "result":{
        "type":"track",
        "page":0,
        "perPage":5,
        "text":"когда переехал не помню",
        "searchRequestId":"music-qa-pr-back-5-2.sas.yp-c.yandex.net-fad8810de1fabcbe0b315e8814178487-1561130980621",
        "tracks":{
            "total":1,
            "perPage":5,
            "order":0,
            "results":[
                {
                    "id":986320,
                    "available":true,
                    "availableAsRbt":false,
                    "availableForPremiumUsers":true,
                    "lyricsAvailable":true,
                    "albums":[
                        {
                            "id":113950,
                            "storageDir":"3a632e62.a.113950",
                            "originalReleaseYear":2002,
                            "year":2002,
                            "version":"",
                            "artists":[

                            ],
                            "coverUri":"avatars.mds.yandex.net/get-music-content/63210/3a632e62.a.113950-2/<тут-должны-быть-два-процента>",
                            "trackCount":15,
                            "genre":"rusrock",
                            "available":true,
                            "contentWarning":"explicit",
                            "availableForPremiumUsers":true,
                            "title":"Пираты XXI века",
                            "trackPosition":{
                                "volume":1,
                                "index":1
                            }
                        }
                    ],
                    "storageDir":"50334_50048e48.1395765.1.986320",
                    "durationMs":167000,
                    "explicit":false,
                    "title":"WWW",
                    "version":"",
                    "artists":[
                        {
                            "id":168851,
                            "cover":{
                                "type":"from-artist-photos",
                                "prefix":"2fbd1b1f.p.168851/",
                                "uri":"avatars.mds.yandex.net/get-music-content/117546/2fbd1b1f.p.168851/<тут-должны-быть-два-процента>"
                            },
                            "composer":false,
                            "name":"Ленинград",
                            "various":false,
                            "decomposed":[

                            ]
                        }
                    ],
                    "poetryCite":{
                        "lines":[
                            {
                                "text":"Когда переехал не помню",
                                "line":0,
                                "tokens":[
                                    {
                                        "start":0,
                                        "end":4,
                                        "matched":true
                                    },
                                    {
                                        "start":6,
                                        "end":13,
                                        "matched":true
                                    },
                                    {
                                        "start":15,
                                        "end":16,
                                        "matched":true
                                    },
                                    {
                                        "start":18,
                                        "end":22,
                                        "matched":true
                                    }
                                ]
                            },
                            {
                                "text":"Наверное был я бухой",
                                "line":1,
                                "tokens":[
                                    {
                                        "start":0,
                                        "end":7,
                                        "matched":false
                                    },
                                    {
                                        "start":9,
                                        "end":11,
                                        "matched":false
                                    },
                                    {
                                        "start":13,
                                        "end":13,
                                        "matched":false
                                    },
                                    {
                                        "start":15,
                                        "end":19,
                                        "matched":false
                                    }
                                ]
                            },
                            {
                                "text":"Мой адрес ни дом и ни улица",
                                "line":2,
                                "tokens":[
                                    {
                                        "start":0,
                                        "end":2,
                                        "matched":false
                                    },
                                    {
                                        "start":4,
                                        "end":8,
                                        "matched":false
                                    },
                                    {
                                        "start":10,
                                        "end":11,
                                        "matched":false
                                    },
                                    {
                                        "start":13,
                                        "end":15,
                                        "matched":false
                                    },
                                    {
                                        "start":17,
                                        "end":17,
                                        "matched":false
                                    },
                                    {
                                        "start":19,
                                        "end":20,
                                        "matched":false
                                    },
                                    {
                                        "start":22,
                                        "end":26,
                                        "matched":false
                                    }
                                ]
                            },
                            {
                                "text":"Мой адрес сегодня такой:",
                                "line":3,
                                "tokens":[
                                    {
                                        "start":0,
                                        "end":2,
                                        "matched":false
                                    },
                                    {
                                        "start":4,
                                        "end":8,
                                        "matched":false
                                    },
                                    {
                                        "start":10,
                                        "end":16,
                                        "matched":false
                                    },
                                    {
                                        "start":18,
                                        "end":22,
                                        "matched":false
                                    }
                                ]
                            }
                        ]
                    },
                    "poetryLoverMatches":[
                        {
                            "begin":0,
                            "end":4,
                            "line":0
                        },
                        {
                            "begin":6,
                            "end":13,
                            "line":0
                        },
                        {
                            "begin":15,
                            "end":16,
                            "line":0
                        },
                        {
                            "begin":18,
                            "end":22,
                            "line":0
                        }
                    ],
                    "regions":[
                        "RUSSIA_PREMIUM",
                        "RUSSIA"
                    ]
                }
            ]
        }
    }
}
`
	var response *MusicSearchResponse
	assert.NoError(t, json.Unmarshal([]byte(responseString), &response))

	expectedResponse := &MusicSearchResponse{
		InvocationInfo: json.RawMessage(`{
        "hostname":"music-qa-pr-back-5-2.sas.yp-c.yandex.net",
        "req-id":"fad8810de1fabcbe0b315e8814178487",
        "exec-duration-millis":"219"
    }`),
		ResponseBody: &MusicSearchResponseBody{
			Text: "когда переехал не помню",
			TracksInfo: TracksInfo{
				Total: 1,
				Tracks: []Track{
					{
						ID:        986320,
						Artists:   []Artist{{Name: "Ленинград"}},
						Available: true,
						Explicit:  false,
						Title:     "WWW",
						PoetryCite: &PoetryCite{
							Lines: []PoetryLine{
								{Text: "Когда переехал не помню", Line: 0},
								{Text: "Наверное был я бухой", Line: 1},
								{Text: "Мой адрес ни дом и ни улица", Line: 2},
								{Text: "Мой адрес сегодня такой:", Line: 3},
							},
						},
					},
				},
			},
		},
	}

	assert.Equal(t, expectedResponse, response)
}

func TestMusicApiSearch_parseSearchResponse(t *testing.T) {
	api := &MusicSearchAPI{testLogger, "doesn't matter", 3, 4}

	t.Run("CorrectResponse", func(t *testing.T) {
		searchResponse := &MusicSearchResponse{
			ResponseBody: &MusicSearchResponseBody{
				Text: "machine",
				TracksInfo: TracksInfo{
					Total: 2,
					Tracks: []Track{
						{
							ID:        500,
							Title:     "Machine",
							Available: true,
							Explicit:  false,
							Artists: []Artist{
								{Name: "Static-X"},
							},
						},
						{
							ID:        600,
							Title:     "Machine lines",
							Available: false,
							Explicit:  true,
							Artists: []Artist{
								{Name: "David O'Dowda"}, {Name: "Rachel Wood"},
							},
						},
					},
				},
			},
		}
		expectedResult := []SearchResult{
			{
				Artist: "Static-X",
				Title:  "Machine",
				URL:    "https://music.yandex.ru/track/500",
			},
			{
				Artist:   "David O'Dowda, Rachel Wood",
				Title:    "Machine lines",
				Explicit: true,
				URL:      "https://music.yandex.ru/track/600",
			},
		}

		searchResult, err := api.parseSearchResponse(searchResponse)
		assert.NoError(t, err)
		assert.Equal(t, expectedResult, searchResult)
	})

	t.Run("ErrorResponse", func(t *testing.T) {
		searchResponse := &MusicSearchResponse{
			ErrorBody: &MusicSearchErrorBody{
				Name:    "Some error",
				Message: "aaand it's gone!",
			},
		}

		searchResult, err := api.parseSearchResponse(searchResponse)
		assert.Error(t, err)
		assert.Nil(t, searchResult)
	})

}
