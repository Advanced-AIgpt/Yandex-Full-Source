package model

import (
	"testing"

	"a.yandex-team.ru/alice/library/go/cipher"
	"github.com/stretchr/testify/assert"
)

func TestNetworkPasswordEncryption(t *testing.T) {
	crypter := &cipher.CrypterMock{}
	someUID := uint64(1)

	network := Network{Password: "some-password"}
	err := network.EncryptPassword(crypter, someUID)
	assert.NoError(t, err)

	err = network.DecryptPassword(crypter, someUID)
	assert.NoError(t, err)
}

func TestNetworksGetOldest(t *testing.T) {
	networks := make(Networks, 0)
	// check not fails on empty slice
	network := networks.GetOldest()
	assert.Empty(t, network)

	networks = append(networks, Network{Updated: 111}, Network{Updated: 222}, Network{Updated: 150})
	network = networks.GetOldest()
	assert.Equal(t, Network{Updated: 111}, network)
}

func TestNetworksContains(t *testing.T) {
	networks := make(Networks, 0)
	// check not fails on empty slice
	network := networks.GetOldest()
	assert.Empty(t, network)

	networks = append(networks, Network{SSID: "some-emoji"}, Network{SSID: "totally-normal-ssid"}, Network{SSID: "kek"})
	assert.False(t, networks.Contains("lol"))
	assert.False(t, networks.Contains("not-some-emoji"))
	assert.True(t, networks.Contains("totally-normal-ssid"))
}
