package ratelimiter

import (
	"context"
	"testing"
	"time"
)

const (
	rateID = "^_^"
)

func CheckNotExceeded(t *testing.T, l RateLimiter) {
	exceeded, err := l.IsRateLimitExceeded(context.Background(), rateID)
	if err != nil {
		t.Fatal(err)
	}
	if exceeded {
		t.Fail()
	}
}

func CheckExceeded(t *testing.T, l RateLimiter) {
	exceeded, err := l.IsRateLimitExceeded(context.Background(), rateID)
	if err != nil {
		t.Fatal(err)
	}
	if !exceeded {
		t.Fail()
	}
}

func TestLimiter(t *testing.T) {
	t.Run("RPS=0", func(t *testing.T) {
		l := New(NewInMemoryStorage(), 0)
		for i := 0; i < 10000; i++ {
			CheckNotExceeded(t, l)
		}
	})
	t.Run("RPS=1", func(t *testing.T) {
		l := New(NewInMemoryStorage(), 1)
		for i := 0; i < 10; i++ {
			if i%2 == 0 {
				CheckNotExceeded(t, l)
			} else {
				CheckExceeded(t, l)
			}
			time.Sleep(501 * time.Millisecond)
		}
	})
	t.Run("RPS=10", func(t *testing.T) {
		l := New(NewInMemoryStorage(), 10)
		for i := 0; i < 10; i++ {
			CheckNotExceeded(t, l)
		}
		for i := 0; i < 10; i++ {
			CheckExceeded(t, l)
		}
	})
}
