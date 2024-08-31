package timetable

import (
	"math/rand"
	"time"
)

// Jitter adds some lag to planned time to mitigate peak load
type Jitter interface {
	// Jit takes plannedTime and applies jitter logic for adding some lag
	// applied time can be a little be greater or less than planned time
	Jit() time.Duration
}

// RandomJitter generates random lag within the given interval
type RandomJitter struct {
	leftBorder  time.Duration
	rightBorder time.Duration
}

// NewRandomJitter creates new instance of time jitter
func NewRandomJitter(leftBorder, rightBorder time.Duration) *RandomJitter {
	return &RandomJitter{
		leftBorder:  leftBorder,
		rightBorder: rightBorder,
	}
}

// Jit generates lag duration, it's a random value within the given interval [leftBorder, rightBorder).
// leftBorder <= plannedTime < rightBorder
func (r *RandomJitter) Jit() time.Duration {
	lag := rand.Int63n(r.rightBorder.Milliseconds()-r.leftBorder.Milliseconds()) + r.leftBorder.Milliseconds()
	return time.Duration(lag) * time.Millisecond
}

// NopJitter implements jitter interface with no effect to planned time
type NopJitter struct{}

func (n *NopJitter) Jit() time.Duration {
	return time.Duration(0)
}
