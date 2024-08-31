package csrf

import "fmt"

type CsrfChecker interface {
	Init(string) error
	CheckToken(string, string, uint64) error
}

type CsrfMock struct{}

func (c *CsrfMock) Init(string) error {
	return nil
}

func (c *CsrfMock) CheckToken(tokenCandidate, yandexuid string, userID uint64) error {
	if tokenCandidate != "valid" {
		return fmt.Errorf("invalid CSRF token: '%s', expected: 'valid'", tokenCandidate)
	}
	return nil
}
