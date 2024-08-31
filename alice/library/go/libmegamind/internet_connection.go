package libmegamind

import (
	"sort"
	"strings"

	"a.yandex-team.ru/alice/megamind/protos/common"
)

type InternetConnection struct {
	*common.TDeviceState_TInternetConnection
}

func (connection InternetConnection) GetSuitableNetwork() (WifiNetwork, bool) {
	// backward compatibility
	if connection.TDeviceState_TInternetConnection == nil {
		return WifiNetwork{}, false
	}
	if connection.Current == nil || connection.Neighbours == nil {
		return WifiNetwork{}, false
	}
	networks := make(WifiNetworks, 0, len(connection.GetNeighbours()))
	for _, network := range connection.GetNeighbours() {
		networks = append(networks, WifiNetwork{network})
	}
	current := WifiNetwork{connection.Current}

	// https://st.yandex-team.ru/IOT-740#5fdb7cdbb806202d3685d341
	if current.Is24Ghz() {
		return current, true
	}
	if len(networks) == 0 {
		return WifiNetwork{}, false
	}
	if !current.Is5Ghz() {
		return WifiNetwork{}, false
	}
	var resLevensteinDist int
	var resNetwork WifiNetwork
	for _, network := range networks {
		if !network.Is24Ghz() {
			continue
		}
		if current.HasSimilarBssid(network) {
			return network, true
		}
		dist := levenshteinDistance(network.GetSsid(), current.GetSsid())
		if resNetwork.TDeviceState_TInternetConnection_TWifiNetwork == nil || resLevensteinDist > dist {
			resNetwork = network
			resLevensteinDist = dist
		}
	}
	if resLevensteinDist <= 3 {
		return resNetwork, true
	}
	return WifiNetwork{}, false
}

type WifiNetwork struct {
	*common.TDeviceState_TInternetConnection_TWifiNetwork
}

type WifiNetworks []WifiNetwork

func (network WifiNetwork) Is24Ghz() bool {
	return network.GetChannel() > 0 && network.GetChannel() < 30
}

func (network WifiNetwork) Is5Ghz() bool {
	return network.GetChannel() > 30
}

func (network WifiNetwork) HasSimilarBssid(other WifiNetwork) bool {
	aBssid := network.GetBssid()
	bBssid := other.GetBssid()
	if aBssid == "" || bBssid == "" {
		return false
	}
	return strings.HasPrefix(aBssid, bBssid[:len(bBssid)-3]) && strings.HasPrefix(bBssid, aBssid[:len(aBssid)-3])
}

func levenshteinDistance(a, b string) int {
	s1 := []rune(a)
	s2 := []rune(b)
	s1len := len(s1)
	s2len := len(s2)
	resColumn := make([]int, len(s1)+1)

	for i := 1; i <= s1len; i++ {
		resColumn[i] = i
	}
	for i := 1; i <= s2len; i++ {
		resColumn[0] = i
		lastkey := i - 1
		for j := 1; j <= s1len; j++ {
			oldkey := resColumn[j]
			var incr int
			if s1[j-1] != s2[i-1] {
				incr = 1
			}
			// minimum from resColumn[j]+1, resColumn[j-1]+1, lastkey+incr
			minArray := []int{resColumn[j] + 1, resColumn[j-1] + 1, lastkey + incr}
			sort.Ints(minArray)
			resColumn[j] = minArray[0]
			lastkey = oldkey
		}
	}
	return resColumn[s1len]
}
