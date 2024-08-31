package cipher

import (
	"encoding/binary"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCBCCrypter(t *testing.T) {
	crypter, err := NewCBCCrypter("6368616e676520746869732070617373")
	assert.NoError(t, err)

	initialString := []byte("just-check-that-crypting")
	myUser := uint64(1111111)
	myUserBytes := make([]byte, 8)
	binary.LittleEndian.PutUint64(myUserBytes, myUser)

	cryptedString, err := crypter.Encrypt(initialString, myUserBytes)
	assert.NoError(t, err)
	assert.NotEqual(t, initialString, cryptedString)

	// crypter with different key
	differentCrypter, err := NewCBCCrypter("6368616e676520746869732070617311")
	assert.NoError(t, err)

	_, err = differentCrypter.Decrypt(cryptedString, myUserBytes)
	assert.Error(t, err)

	decryptedString, err := crypter.Decrypt(cryptedString, myUserBytes)
	assert.NoError(t, err)

	assert.Equal(t, initialString, decryptedString)
}
