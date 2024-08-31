package datasync

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestPersonalityAddressesResponseUnmarshalling(t *testing.T) {
	datasyncAnswer := `{
	"items": [{
		"last_used": "2021-03-23T13:11:55.917000+00:00",
		"address_id": "work",
		"tags": ["work"],
		"title": "улица Льва Толстого, 16",
		"modified": "2021-03-23T13:11:55.917000+00:00",
		"longitude": 37.58709335,
		"created": "2021-03-23T13:11:55.917000+00:00",
		"mined_attributes": [],
		"address_line_short": "улица Льва Толстого, 16",
		"latitude": 55.73397446,
		"address_line": "Россия, Москва, улица Льва Толстого, 16"
	}, {
		"last_used": "2021-03-23T13:30:01.939000+00:00",
		"address_id": "home",
		"tags": ["home"],
		"title": "Белореченская улица, 28к1",
		"modified": "2021-03-23T13:30:01.939000+00:00",
		"longitude": 37.76940155,
		"created": "2021-03-23T13:30:01.939000+00:00",
		"mined_attributes": [],
		"address_line_short": "Белореченская улица, 28к1",
		"latitude": 55.66388702,
		"address_line": "Россия, Москва, Белореченская улица, 28к1"
	}],
	"total": 2,
	"limit": 20,
	"offset": 0
}`
	var response PersonalityAddressesResponse
	err := json.Unmarshal([]byte(datasyncAnswer), &response)
	assert.NoError(t, err)
	assert.Equal(t, 2, len(response.Items))
	assert.Equal(t, "Россия, Москва, улица Льва Толстого, 16", response.Items[0].NormalizedAddress())
	assert.Equal(t, "Улица Льва Толстого, 16", response.Items[0].NormalizedShortAddress())
	assert.Equal(t, 37.58709335, response.Items[0].Longitude)
	assert.Equal(t, 55.73397446, response.Items[0].Latitude)
	assert.Equal(t, "Россия, Москва, Белореченская улица, 28к1", response.Items[1].NormalizedAddress())
	assert.Equal(t, "Белореченская улица, 28к1", response.Items[1].NormalizedShortAddress())
	assert.Equal(t, 37.76940155, response.Items[1].Longitude)
	assert.Equal(t, 55.66388702, response.Items[1].Latitude)
}
