package queue

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func Test_ConstantDelay(t *testing.T) {
	duration := 4 * time.Second
	delay := ConstantDelay(duration)
	inputs := []struct {
		Attempt  int
		Expected time.Duration
	}{
		{
			Attempt:  0,
			Expected: duration,
		},
		{
			Attempt:  1,
			Expected: duration,
		},
		{
			Attempt:  2,
			Expected: duration,
		},
		{
			Attempt:  3,
			Expected: duration,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("Constant delay, attempt %d", input.Attempt), func(t *testing.T) {
			assert.Equal(t, input.Expected, delay(input.Attempt))
		})
	}
}

func Test_LinearDelay(t *testing.T) {
	duration := 4 * time.Second
	delay := LinearDelay(duration)
	inputs := []struct {
		Attempt  int
		Expected time.Duration
	}{
		{
			Attempt:  0,
			Expected: duration,
		},
		{
			Attempt:  1,
			Expected: 2 * duration,
		},
		{
			Attempt:  2,
			Expected: 3 * duration,
		},
		{
			Attempt:  3,
			Expected: 4 * duration,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("Linear delay, attempt %d", input.Attempt), func(t *testing.T) {
			assert.Equal(t, input.Expected, delay(input.Attempt))
		})
	}
}

func Test_PolynomialDelay(t *testing.T) {
	duration := 4 * time.Second
	delay := PolynomialDelay(duration, 2)
	inputs := []struct {
		Attempt  int
		Expected time.Duration
	}{
		{
			Attempt:  0,
			Expected: duration,
		},
		{
			Attempt:  1,
			Expected: 2 * duration,
		},
		{
			Attempt:  2,
			Expected: 4 * duration,
		},
		{
			Attempt:  3,
			Expected: 8 * duration,
		},
		{
			Attempt:  4,
			Expected: 16 * duration,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("Polynomial delay, attempt %d", input.Attempt), func(t *testing.T) {
			assert.Equal(t, input.Expected, delay(input.Attempt))
		})
	}
}

func Test_CompoundRetryPolicy(t *testing.T) {
	p := NewCompoundPolicy(
		SimpleRetryPolicy{
			Count: 3,
			Delay: ConstantDelay(30 * time.Second),
		},
		SimpleRetryPolicy{
			Count: 10,
			Delay: LinearDelay(1 * time.Minute),
		},
	)

	assert.Equal(t, 13, p.GetTotalCount())
	assert.Equal(t, 30*time.Second, p.GetDelay(0))
	assert.Equal(t, 30*time.Second, p.GetDelay(1))
	assert.Equal(t, 30*time.Second, p.GetDelay(2))
	assert.Equal(t, 1*time.Minute, p.GetDelay(3))
	assert.Equal(t, 2*time.Minute, p.GetDelay(4))
	assert.Equal(t, 3*time.Minute, p.GetDelay(5))
	assert.Equal(t, 7*time.Minute, p.GetDelay(9))
}
