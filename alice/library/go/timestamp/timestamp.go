package timestamp

import (
	"context"
	"math"
	"strconv"
	"time"

	"golang.org/x/xerrors"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(PastTimestamp)

// Timestamp in seconds.
// float64 allows for some precision (e.g. PastTimestamp 1.001 is 1001 ms)
type PastTimestamp float64

// https://st.yandex-team.ru/IOT-731
func (pts PastTimestamp) MarshalJSON() ([]byte, error) {
	fpts := float64(pts)
	if fpts == math.Trunc(fpts) {
		// will print trailing zero
		return []byte(strconv.FormatFloat(fpts, 'f', 1, 64)), nil
	}
	// will use smallest number of digits needed
	return []byte(strconv.FormatFloat(fpts, 'f', -1, 64)), nil
}

func FromProto(timestamp *timestamppb.Timestamp) PastTimestamp {
	return FromTime(timestamp.AsTime())
}

func FromNano(nanoseconds uint64) PastTimestamp {
	return PastTimestamp(float64(nanoseconds) / 1e9)
}

func FromMilli(milliseconds uint64) PastTimestamp {
	return PastTimestamp(float64(milliseconds) / 1e3)
}

func FromMicro(microseconds uint64) PastTimestamp {
	return PastTimestamp(float64(microseconds) / 1e6)
}

func FromTime(time time.Time) PastTimestamp {
	return PastTimestamp(time.UnixNano()) / 1e9
}

func Now() PastTimestamp {
	return FromTime(time.Now())
}

func CurrentTimestampCtx(ctx context.Context) PastTimestamp {
	timestamper, err := TimestamperFromContext(ctx)
	if err != nil {
		return Now()
	}
	return timestamper.CurrentTimestamp()
}

func (pts PastTimestamp) Validate(vctx *valid.ValidationCtx) (bool, error) {
	// 1 minute is added to avoid clock dissynchronization
	now := PastTimestamp(float64(time.Now().Add(time.Minute).UnixNano()) * 1.e-9)
	if pts > now {
		return false, xerrors.Errorf("value is more than current timestamp %f", now)
	}

	return false, nil
}

func (pts PastTimestamp) Unix() int64 {
	return int64(pts)
}

func (pts PastTimestamp) UnixMicro() int64 {
	return pts.UnixNano() / 1e3
}

func (pts PastTimestamp) UnixNano() int64 {
	return pts.AsTime().UnixNano()
}

func (pts PastTimestamp) YdbTimestamp() uint64 {
	return uint64(pts.UnixMicro()) // ydb PastTimestamp is microseconds
}

func (pts PastTimestamp) AsTime() time.Time {
	sec, dec := math.Modf(float64(pts))
	return time.Unix(int64(sec), int64(dec*(1e9)))
}

func (pts PastTimestamp) RoundToSeconds() PastTimestamp {
	val := math.Round(float64(pts))
	return PastTimestamp(val)
}

func (pts PastTimestamp) AsDuration() time.Duration {
	return time.Duration(pts.UnixNano())
}

func (pts PastTimestamp) Add(duration time.Duration) PastTimestamp {
	return FromTime(pts.AsTime().Add(duration))
}

func AdjustChildRequestTimeout(ctx context.Context, logger log.Logger, timeout time.Duration) time.Duration {
	ts, err := TimestamperFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, logger, "failed to get timestamper from context, falling back to default timeout: %dms", timeout.Milliseconds())
		return timeout
	}
	requestDuration := ts.CurrentTimestamp() - ts.CreatedTimestamp()
	return timeout - requestDuration.AsDuration()
}

// ComputeTimeout returns the time left to process a request.
// If the context passed contains a timestamper, it returns maxTimeout - timePassed,
// where timePassed is the time elapsed since timestamper initialization.
// Otherwise, an error is returned.
func ComputeTimeout(timestamperCtx context.Context, maxTimeout time.Duration) (time.Duration, error) {
	ts, err := TimestamperFromContext(timestamperCtx)
	if err == nil {
		requestDuration := ts.CurrentTimestamp() - ts.CreatedTimestamp()
		return maxTimeout - requestDuration.AsDuration(), nil
	}

	return time.Duration(0), err
}
