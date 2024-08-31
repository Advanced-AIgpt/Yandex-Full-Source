package generic

import (
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCounter(t *testing.T) {
	counter := &Counter{}

	times := 1000
	var wg sync.WaitGroup
	wg.Add(times)
	for i := 0; i < times; i++ {
		go func() {
			defer wg.Done()
			counter.Add(1)
		}()
	}

	wg.Wait()
	assert.Equal(t, int64(times), counter.Value())
}
