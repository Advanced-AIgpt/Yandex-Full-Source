package metrics

type CounterMock struct {
	name string
}

func (cm CounterMock) Inc() {}

func (cm CounterMock) Add(delta int64) {}
