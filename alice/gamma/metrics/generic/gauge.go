package generic

import (
	"math"
	"sync/atomic"
)

type Gauge struct {
	bits uint64
}

func NewGauge(value float64) *Gauge {
	gauge := &Gauge{}
	gauge.Set(value)
	return gauge
}

func (g *Gauge) Set(value float64) {
	atomic.StoreUint64(&g.bits, math.Float64bits(value))
}

func (g *Gauge) Add(delta float64) {
	for {
		var (
			oldBits = atomic.LoadUint64(&g.bits)
			newf    = math.Float64frombits(oldBits) + delta
			newBits = math.Float64bits(newf)
		)
		if atomic.CompareAndSwapUint64(&g.bits, oldBits, newBits) {
			break
		}
	}
}

func (g *Gauge) Value() float64 {
	return math.Float64frombits(atomic.LoadUint64(&g.bits))
}
