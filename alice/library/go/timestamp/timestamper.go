package timestamp

const (
	CreatedTimestampMock PastTimestamp = 1
	CurrentTimestampMock PastTimestamp = 1
)

type ITimestamper interface {
	CreatedTimestamp() PastTimestamp
	CurrentTimestamp() PastTimestamp
}

type TimestamperMock struct {
	CreatedTimestampValue PastTimestamp
	CurrentTimestampValue PastTimestamp
}

func NewMockTimestamper() *TimestamperMock {
	return &TimestamperMock{CreatedTimestampMock, CurrentTimestampMock}
}

func (tm *TimestamperMock) CurrentTimestamp() PastTimestamp {
	return tm.CurrentTimestampValue // created for testing purposes, s.Equal breaks if updated field is not equal
}

func (tm *TimestamperMock) CreatedTimestamp() PastTimestamp {
	return tm.CreatedTimestampValue // created for testing purposes, s.Equal breaks if updated field is not equal
}

func (tm *TimestamperMock) SetCurrentTimestamp(ts PastTimestamp) {
	tm.CurrentTimestampValue = ts
}

func (tm *TimestamperMock) WithCurrentTimestamp(ts PastTimestamp) *TimestamperMock {
	tm.CurrentTimestampValue = ts
	return tm
}

func (tm *TimestamperMock) WithCreatedTimestamp(ts PastTimestamp) *TimestamperMock {
	tm.CreatedTimestampValue = ts
	return tm
}

type Timestamper struct {
	createdTimestamp PastTimestamp
}

func NewTimestamper() Timestamper {
	return Timestamper{
		createdTimestamp: Now(),
	}
}

func (tm Timestamper) CurrentTimestamp() PastTimestamp {
	return Now()
}

func (tm Timestamper) CreatedTimestamp() PastTimestamp {
	return tm.createdTimestamp
}

type ITimestamperFactory interface {
	NewTimestamper() ITimestamper
}

type TimestamperFactory struct{}

func (tf TimestamperFactory) NewTimestamper() ITimestamper {
	return NewTimestamper()
}

type TimestamperFactoryMock struct {
	ts PastTimestamp
}

func NewTimestamperFactoryMock() *TimestamperFactoryMock {
	return &TimestamperFactoryMock{ts: 1}
}

func (tf *TimestamperFactoryMock) NewTimestamper() ITimestamper {
	return NewMockTimestamper().WithCurrentTimestamp(tf.ts)
}

func (tf *TimestamperFactoryMock) WithCreatedTimestamp(ts PastTimestamp) {
	tf.ts = ts
}
