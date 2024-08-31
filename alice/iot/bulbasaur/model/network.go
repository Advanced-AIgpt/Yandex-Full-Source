package model

import (
	"encoding/binary"

	"a.yandex-team.ru/alice/library/go/cipher"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type Network struct {
	SSID     string
	Password string
	Updated  timestamp.PastTimestamp
}

func (n *Network) EncryptPassword(crypter cipher.ICrypter, yandexUID uint64) error {
	yandexUIDBytes := make([]byte, 8)
	binary.LittleEndian.PutUint64(yandexUIDBytes, yandexUID)
	encrypted, err := crypter.Encrypt([]byte(n.Password), yandexUIDBytes)
	if err != nil {
		return err
	}
	n.Password = string(encrypted)
	return nil
}

func (n *Network) DecryptPassword(crypter cipher.ICrypter, yandexUID uint64) error {
	yandexUIDBytes := make([]byte, 8)
	binary.LittleEndian.PutUint64(yandexUIDBytes, yandexUID)
	decrypted, err := crypter.Decrypt([]byte(n.Password), yandexUIDBytes)
	if err != nil {
		return err
	}
	n.Password = string(decrypted)
	return nil
}

type Networks []Network

func (n Networks) GetOldest() Network {
	if len(n) == 0 {
		return Network{}
	}
	oldestIndex := 0
	for i := range n {
		if n[i].Updated < n[oldestIndex].Updated {
			oldestIndex = i
		}
	}
	return n[oldestIndex]
}

func (n Networks) Contains(SSID string) bool {
	for _, network := range n {
		if network.SSID == SSID {
			return true
		}
	}
	return false
}
