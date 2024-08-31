package cipher

type ICrypter interface {
	Encrypt(data []byte, IV []byte) ([]byte, error)
	Decrypt(data []byte, IV []byte) ([]byte, error)
}

type CrypterMock struct{}

func (m *CrypterMock) Encrypt(data []byte, IV []byte) ([]byte, error) {
	return data, nil
}

func (m *CrypterMock) Decrypt(data []byte, IV []byte) ([]byte, error) {
	return data, nil
}
