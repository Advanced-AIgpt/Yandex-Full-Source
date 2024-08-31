package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/library/go/geosuggest"
	"github.com/stretchr/testify/assert"
)

func TestGeosuggestsViewFromGeosuggestResponse(t *testing.T) {
	geosuggestResponse := geosuggest.GeosuggestFromAddressResponse{
		Part: "Москва Давыдова 16",
		Results: []geosuggest.Geosuggest{
			{
				RawCoordinates: "37.559948,55.403053",
				RawAddress:     "Россия, Московская область, Подольск, улица Давыдова, 16",
			},
		},
	}
	geosuggestResponse.Results[0].Title.RawShortAddress = "Улица Давыдова, 16"
	var geosuggestsView GeosuggestsView
	geosuggestsView.FromGeosuggestResponse(geosuggestResponse)
	assert.Equal(t, "Улица Давыдова, 16", geosuggestsView.Suggests[0].ShortAddress)
	assert.Equal(t, "Россия, Московская область, Подольск, улица Давыдова, 16", geosuggestsView.Suggests[0].Address)
}
