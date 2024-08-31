package queue

import (
	"math"
	"time"
)

type RetryPolicy interface {
	GetTotalCount() int
	GetDelay(attempt int) time.Duration
	GetRecoverErrWrapper() RecoverErrWrapper
}

type SimpleRetryPolicy struct {
	Count             int
	Delay             DelayPolicy
	RecoverErrWrapper RecoverErrWrapper
}

func (s SimpleRetryPolicy) GetTotalCount() int {
	return s.Count
}

func (s SimpleRetryPolicy) GetDelay(attempt int) time.Duration {
	return s.Delay(attempt)
}

func (s SimpleRetryPolicy) GetRecoverErrWrapper() RecoverErrWrapper {
	return s.RecoverErrWrapper
}

type CompoundRetryPolicy struct {
	Policies []RetryPolicy
}

func NewCompoundPolicy(policies ...RetryPolicy) *CompoundRetryPolicy {
	return &CompoundRetryPolicy{Policies: policies}
}

func (c CompoundRetryPolicy) GetTotalCount() int {
	count := 0
	for _, p := range c.Policies {
		count += p.GetTotalCount()
	}
	return count
}

func (c CompoundRetryPolicy) GetDelay(attempt int) time.Duration {
	for _, p := range c.Policies {
		if attempt < p.GetTotalCount() {
			return p.GetDelay(attempt)
		}
		attempt -= p.GetTotalCount()
	}
	return 0
}

func (c CompoundRetryPolicy) GetRecoverErrWrapper() RecoverErrWrapper {
	for _, p := range c.Policies {
		if wrapper := p.GetRecoverErrWrapper(); wrapper != nil {
			return wrapper
		}
	}
	return nil
}

type DelayPolicy func(int) time.Duration

func ConstantDelay(duration time.Duration) DelayPolicy {
	return func(_ int) time.Duration {
		return duration
	}
}

func LinearDelay(increment time.Duration) DelayPolicy {
	return func(attempt int) time.Duration {
		return time.Duration(attempt+1) * increment
	}
}

func PolynomialDelay(initialDelay time.Duration, coefficient int) DelayPolicy {
	return func(attempt int) time.Duration {
		return initialDelay * time.Duration(math.Pow(float64(coefficient), float64(attempt)))
	}
}

type RecoverErrWrapper func(error) error

// RecoverWithFailAndResubmit create wrapper with error for resubmitting task to the queue with given delay
func RecoverWithFailAndResubmit(delay time.Duration) RecoverErrWrapper {
	return func(err error) error {
		return NewFailAndResubmitTaskError(delay, err)
	}
}
