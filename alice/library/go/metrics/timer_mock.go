package metrics

import (
	"time"
)

type TimerMock struct {
	name string
}

func (tm TimerMock) RecordDuration(value time.Duration) {}
