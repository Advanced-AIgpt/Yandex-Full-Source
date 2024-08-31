package tuya

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type TokenProvider struct {
	sync.RWMutex
	token TuyaToken

	maxRetries int
	provider   func(context.Context) (TuyaToken, error)
}

func newTokenProvider(ctx context.Context, maxRetries int, provider func(ctx context.Context) (TuyaToken, error)) *TokenProvider {
	tokenProvider := TokenProvider{
		provider:   provider,
		maxRetries: maxRetries,
	}
	_, _ = tokenProvider.refreshToken(ctx)
	return &tokenProvider
}

func (p *TokenProvider) GetToken(ctx context.Context) (result string, err error) {
	p.RLock()
	defer func(token TuyaToken) {
		if !token.isValid() {
			result, err = p.refreshToken(ctx)
		}
	}(p.token)
	defer p.RUnlock()
	return p.token.value, nil
}

func (p *TokenProvider) refreshToken(ctx context.Context) (string, error) {
	p.Lock()
	defer p.Unlock()

	var token TuyaToken
	var err error
	for i := 0; !p.token.isValid() && i < p.maxRetries; i++ {
		if token, err = p.provider(ctx); err == nil {
			p.token = token
		}
	}
	if !p.token.isValid() {
		return "", xerrors.Errorf("failed to refresh Tuya token: %w", err)
	}
	return p.token.value, nil
}

func (p *TokenProvider) invalidateToken(invalidToken string) {
	p.Lock()
	defer p.Unlock()

	if p.token.value == invalidToken {
		p.token.expireTimestamp = 0
	}
}

type TuyaToken struct {
	value           string
	expireTimestamp int64
}

func (t *TuyaToken) isValid() bool {
	return len(t.value) > 0 && t.expireTimestamp > time.Now().UnixNano()
}
