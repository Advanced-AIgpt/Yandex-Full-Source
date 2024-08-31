package generic

import (
	"sync/atomic"
)

type Counter struct {
	value int64
}

func NewCounter(v int64) *Counter {
	return &Counter{value: v}
}

func (c *Counter) Inc() {
	c.Add(1)
}

func (c *Counter) Add(delta int64) {
	atomic.AddInt64(&c.value, delta)
}

func (c *Counter) Value() int64 {
	return atomic.LoadInt64(&c.value)
}
