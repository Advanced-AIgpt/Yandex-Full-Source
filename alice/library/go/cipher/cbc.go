package cipher

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"crypto/hmac"
	"crypto/sha256"
	"encoding/hex"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//
type CBCCrypter struct {
	key []byte
}

func (c *CBCCrypter) Encrypt(data []byte, IV []byte) ([]byte, error) {
	paddedData := pkcs5Padding(data, aes.BlockSize)
	block, err := aes.NewCipher(c.key)
	if err != nil {
		return nil, xerrors.Errorf("failed to create block in CBCCrypter: %v", err)
	}

	// assumed that IV is no longer than blockSize and pad it
	paddedIV := make([]byte, aes.BlockSize)
	copy(paddedIV[:], IV)

	cipherText := make([]byte, len(paddedData))

	// crypt it
	mode := cipher.NewCBCEncrypter(block, paddedIV)
	mode.CryptBlocks(cipherText, paddedData)
	return macPadding(cipherText, c.key), nil
}

func (c *CBCCrypter) Decrypt(data []byte, IV []byte) ([]byte, error) {
	block, err := aes.NewCipher(c.key)
	if err != nil {
		return nil, xerrors.Errorf("failed to create block in CBCCrypter: %v", err)
	}
	if len(data) < aes.BlockSize {
		return nil, xerrors.Errorf("failed to decrypt data: data is too short")
	}
	if (len(data) % aes.BlockSize) != 0 {
		return nil, xerrors.Errorf("failed to decrypt data: data length is not a multiple to 256-bit")
	}

	// assumed that IV is no longer than blockSize and pad it
	paddedIV := make([]byte, aes.BlockSize)
	copy(paddedIV[:], IV)

	// shrink MAC from data to decrypt
	cipherText, valid := macUnPadding(data, c.key)
	if !valid {
		return nil, xerrors.Errorf("failed to decrypt data: MAC is not valid")
	}

	// decrypt it
	mode := cipher.NewCBCDecrypter(block, paddedIV)
	mode.CryptBlocks(cipherText, cipherText)

	unpaddedData := pkcs5UnPadding(cipherText)
	return unpaddedData, nil
}

func NewCBCCrypter(key string) (CBCCrypter, error) {
	secretKey, err := hex.DecodeString(key)
	if err != nil {
		return CBCCrypter{}, xerrors.Errorf("failed to create new crypter: %v", err)
	}
	return CBCCrypter{key: secretKey}, nil
}

func pkcs5Padding(src []byte, blockSize int) []byte {
	padding := blockSize - len(src)%blockSize
	padtext := bytes.Repeat([]byte{byte(padding)}, padding)
	return append(src, padtext...)
}

func pkcs5UnPadding(src []byte) []byte {
	length := len(src)
	unpadding := int(src[length-1])
	return src[:(length - unpadding)]
}

func macPadding(data, key []byte) []byte {
	mac := hmac.New(sha256.New, key)
	mac.Write(data)
	cryptedMac := mac.Sum(nil)
	return append(data, cryptedMac...)
}

func macUnPadding(src []byte, key []byte) ([]byte, bool) {
	if len(src) < sha256.Size {
		return nil, false
	}
	data, mac := src[:len(src)-sha256.Size], src[len(src)-sha256.Size:]
	if valid := validMAC(data, mac, key); !valid {
		return nil, false
	}
	return data, true
}

func validMAC(data, dataMAC, key []byte) bool {
	mac := hmac.New(sha256.New, key)
	mac.Write(data)
	expectedMAC := mac.Sum(nil)
	return hmac.Equal(dataMAC, expectedMAC)
}
