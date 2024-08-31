package tuya

import (
	"context"
	"strconv"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestToken(t *testing.T) {
	t.Run("Validation", func(t *testing.T) {
		token := TuyaToken{
			value:           "",
			expireTimestamp: time.Now().Add(time.Hour).UnixNano(),
		}
		assert.False(t, token.isValid())

		token.value = "ssss"
		assert.True(t, token.isValid())

		token.expireTimestamp = 0
		assert.False(t, token.isValid())
	})

	t.Run("GetToken", func(t *testing.T) {
		counter := int64(0)
		ctx := context.Background()

		providerFunc := func(ctx context.Context) (TuyaToken, error) {
			atomic.AddInt64(&counter, 1)
			return TuyaToken{value: strconv.FormatInt(counter, 10), expireTimestamp: time.Now().Add(time.Hour).UnixNano()}, nil
		}

		tokenProvider := newTokenProvider(ctx, 10, providerFunc)

		goroutines := 1000
		var wg sync.WaitGroup
		wg.Add(goroutines)
		for i := 0; i < goroutines; i++ {
			go func() {
				token, err := tokenProvider.GetToken(ctx)
				assert.NoError(t, err)
				assert.Equal(t, "1", token)
				wg.Done()
			}()
		}
		wg.Wait()

		tokenProvider.invalidateToken(tokenProvider.token.value)
		wg.Add(goroutines)
		for i := 0; i < goroutines; i++ {
			go func() {
				token, err := tokenProvider.GetToken(ctx)
				assert.NoError(t, err)
				assert.Equal(t, "2", token)
				wg.Done()
			}()
		}
		wg.Wait()
	})

	t.Run("ConcurrentInvalidate", func(t *testing.T) {
		counter := int64(0)
		ctx := context.Background()

		providerFunc := func(ctx context.Context) (TuyaToken, error) {
			atomic.AddInt64(&counter, 1)
			return TuyaToken{value: strconv.FormatInt(counter, 10), expireTimestamp: time.Now().Add(time.Hour).UnixNano()}, nil
		}

		tokenProvider := newTokenProvider(ctx, 10, providerFunc)

		var wg sync.WaitGroup
		wg.Add(2)

		workStart := make(chan bool)
		go func() { // slow goroutine
			defer wg.Done()
			token, err := tokenProvider.GetToken(ctx)
			assert.NoError(t, err)
			assert.Equal(t, "1", token)

			workStart <- true
			// this chan wait emulates long work with "valid" token
			// another goroutine is refreshing token during work
			<-workStart

			// consider that this goroutine thinks her token is invalid & tries to invalidate token
			tokenProvider.invalidateToken(token) // this should fail, as we have new token now
			token, err = tokenProvider.GetToken(ctx)
			assert.NoError(t, err)
			assert.Equal(t, "2", token)
		}()

		go func() { // fast goroutine
			defer wg.Done()

			<-workStart

			tokenProvider.invalidateToken("1") // invalidate token
			token, err := tokenProvider.GetToken(ctx)
			assert.NoError(t, err)
			assert.Equal(t, "2", token)

			workStart <- true
		}()

		wg.Wait()
	})
}
