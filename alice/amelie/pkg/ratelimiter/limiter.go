package ratelimiter

import (
	"context"
	"time"
)

type Rate struct {
	ID    string      `json:"rate_id" bson:"rate_id"`
	Times []time.Time `json:"times" bson:"times"`
}

type RateLimiter interface {
	SetPeriod(period time.Duration) RateLimiter
	IsRateLimitExceeded(ctx context.Context, id string) (exceeded bool, err error)
}

type Storage interface {
	Get(ctx context.Context, id string) (Rate, error)
	Update(ctx context.Context, rate Rate) error
}

type limiter struct {
	storage   Storage
	rateLimit uint
	period    time.Duration
}

func (l *limiter) SetPeriod(period time.Duration) RateLimiter {
	l.period = period
	return l
}

func (l *limiter) IsRateLimitExceeded(ctx context.Context, id string) (bool, error) {
	if l.rateLimit == 0 {
		return false, nil
	}
	now := time.Now()
	rate, err := l.storage.Get(ctx, id)
	if err != nil {
		return false, err
	}
	for len(rate.Times) > 0 {
		delta := now.Sub(rate.Times[0])
		if delta > l.period {
			rate.Times = rate.Times[1:]
		} else {
			break
		}
	}
	if uint(len(rate.Times)) < l.rateLimit {
		rate.Times = append(rate.Times, now)
		return false, l.storage.Update(ctx, rate)
	}
	return true, nil
}

func New(storage Storage, rateLimit uint) RateLimiter {
	return &limiter{
		storage:   storage,
		rateLimit: rateLimit,
		period:    time.Second,
	}
}

type storage struct {
	m map[string]Rate
}

func (s *storage) Get(ctx context.Context, id string) (Rate, error) {
	if r, ok := s.m[id]; ok {
		return r, nil
	}
	return Rate{ID: id}, nil
}

func (s *storage) Update(ctx context.Context, rate Rate) error {
	s.m[rate.ID] = rate
	return nil
}

func NewInMemoryStorage() Storage {
	return &storage{
		m: map[string]Rate{},
	}
}
