package libmegamind

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/ptr"
)

func newNetwork(ssid string, bssid string, channel int32) *common.TDeviceState_TInternetConnection_TWifiNetwork {
	return &common.TDeviceState_TInternetConnection_TWifiNetwork{
		Ssid:    ptr.String(ssid),
		Bssid:   ptr.String(bssid),
		Channel: ptr.Int32(channel),
	}
}

func TestLevensteinDistance(t *testing.T) {
	assert.Equal(t, levenshteinDistance("Вайфай№1_2.4Ghz", "Вайфай№1_5Ghz"), 3)
	assert.Equal(t, levenshteinDistance("my_wifi", "my_wifi_5G"), 3)
	assert.Equal(t, levenshteinDistance("Dlink-890L_24G", "Dlink-890L_5G"), 2)
	assert.Equal(t, levenshteinDistance("ASUS-2.4", "ASUS-5G"), 3)
	assert.Equal(t, levenshteinDistance("Xiaomi_Station_5G", "yandex_ats_01_2"), 14)
	assert.Equal(t, levenshteinDistance("yandex_ats_01_2", "Xiaomi_Station_5G"), 14)
}

func TestSuitableNetwork(t *testing.T) {
	connectionType := common.TDeviceState_TInternetConnection_Wifi_5GHz
	neighbours := []*common.TDeviceState_TInternetConnection_TWifiNetwork{
		newNetwork("Xiaomi_Station_2.4G", "40:31:3c:e2:23:61", 10),
		newNetwork("Xiaomi_Station_5G", "40:31:3c:e2:23:62", 149),
		newNetwork("zomb_grape_AP_5G", "04:d4:c4:c2:d4:cc", 44),
		newNetwork("Asus-Station_2.4G", "b0:6e:bf:3e:a1:60", 2),
		newNetwork("MobCert", "0c:27:24:87:c6:55", 6),
		newNetwork("ZMI_8CDF", "74:a3:4a:a2:8c:df", 8),
		newNetwork("MobDevInternet", "0c:68:03:af:e1:a9", 56),
		newNetwork("MobTest", "0c:68:03:af:e1:a8", 56),
		newNetwork("Guests", "0c:68:03:af:e1:ab", 56),
		newNetwork("Yandex", "0c:68:03:af:e1:af", 56),
		newNetwork("PDAS", "0c:68:03:af:e1:ae", 56),
		newNetwork("[TV] Samsung 6 Series (49)", "b8:bc:5b:ef:49:3b", 1),
		newNetwork("Asus-Station_5G", "b0:6e:bf:3e:a1:64", 36),
		newNetwork("yandex_ats_01_2", "04:d9:f5:73:ca:98", 11),
		newNetwork("zomb_grape_AP", "04:d4:c4:c2:d4:c8", 6),
		newNetwork("yandex_ats_01_5", "04:d9:f5:73:ca:9c", 60),
	}
	xiaomi5 := newNetwork("Xiaomi_Station_5G", "02:00:00:00:00:00", 149)
	xiaomi24 := newNetwork("Xiaomi_Station_2.4G", "40:31:3c:e2:23:61", 10)
	asus5 := newNetwork("Asus-Station_5G", "b0:6e:bf:3e:a1:64", 36)
	asus24 := newNetwork("Asus-Station_2.4G", "b0:6e:bf:3e:a1:60", 2)
	pdas := newNetwork("PDAS", "0c:68:03:af:e1:ae", 56)

	type testCase struct {
		current          *common.TDeviceState_TInternetConnection_TWifiNetwork
		expectedSuitable *common.TDeviceState_TInternetConnection_TWifiNetwork
		suitableExists   bool
	}
	testCases := []testCase{
		{
			current:          xiaomi5,
			expectedSuitable: xiaomi24,
			suitableExists:   true,
		},
		{
			current:          xiaomi24,
			expectedSuitable: xiaomi24,
			suitableExists:   true,
		},
		{
			current:          asus5,
			expectedSuitable: asus24,
			suitableExists:   true,
		},
		{
			current:          asus24,
			expectedSuitable: asus24,
			suitableExists:   true,
		},
		{
			current:          pdas,
			expectedSuitable: nil,
			suitableExists:   false,
		},
	}
	for _, tc := range testCases {
		testInternetConnection := InternetConnection{
			TDeviceState_TInternetConnection: &common.TDeviceState_TInternetConnection{
				Type:       &connectionType,
				Neighbours: neighbours,
				Current:    tc.current,
			},
		}
		expectedSuitableNetwork := WifiNetwork{tc.expectedSuitable}
		actual, exists := testInternetConnection.GetSuitableNetwork()
		assert.Equal(t, tc.suitableExists, exists)
		assert.Equal(t, expectedSuitableNetwork, actual)
	}
}

func TestHasSimilarBssid(t *testing.T) {
	xiaomi24 := WifiNetwork{newNetwork("Xiaomi_Station_2.4G", "40:31:3c:e2:23:61", 10)}
	xiaomi5 := WifiNetwork{newNetwork("Xiaomi_Station_5G", "40:31:3c:e2:23:62", 149)}
	asus24 := WifiNetwork{newNetwork("Asus-Station_2.4G", "b0:6e:bf:3e:a1:60", 2)}
	asus5 := WifiNetwork{newNetwork("Asus-Station_5G", "b0:6e:bf:3e:a1:64", 36)}
	yandexAts24 := WifiNetwork{newNetwork("yandex_ats_01_2", "04:d9:f5:73:ca:98", 11)}
	yandexAts5 := WifiNetwork{newNetwork("yandex_ats_01_5", "04:d9:f5:73:ca:9c", 60)}
	pdas := WifiNetwork{newNetwork("PDAS", "0c:68:03:af:e1:ae", 56)}
	guests := WifiNetwork{newNetwork("Guests", "0c:68:03:af:e1:ab", 56)}
	assert.True(t, pdas.HasSimilarBssid(guests))
	assert.True(t, guests.HasSimilarBssid(pdas))
	assert.True(t, xiaomi24.HasSimilarBssid(xiaomi5))
	assert.True(t, xiaomi5.HasSimilarBssid(xiaomi24))
	assert.True(t, asus24.HasSimilarBssid(asus5))
	assert.True(t, asus5.HasSimilarBssid(asus24))
	assert.True(t, yandexAts24.HasSimilarBssid(yandexAts5))
	assert.True(t, yandexAts5.HasSimilarBssid(yandexAts24))
	assert.False(t, pdas.HasSimilarBssid(xiaomi24))
	assert.False(t, xiaomi24.HasSimilarBssid(asus24))
	assert.False(t, yandexAts24.HasSimilarBssid(xiaomi24))
}
