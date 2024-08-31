package contexter

import (
	"context"
	"time"
)

type noCancel struct {
	ctx context.Context
}

func (nc noCancel) Deadline() (deadline time.Time, ok bool) {
	return time.Time{}, false
}

func (nc noCancel) Done() <-chan struct{} {
	return nil
}

func (nc noCancel) Err() error {
	return nil
}

func (nc noCancel) Value(key interface{}) interface{} {
	return nc.ctx.Value(key)
}

func NoCancel(ctx context.Context) context.Context {
	return noCancel{ctx: ctx}
}
