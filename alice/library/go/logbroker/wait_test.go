package logbroker

import (
	"context"
	"github.com/stretchr/testify/assert"
	"testing"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestWaitCommitWithNotification(t *testing.T) {
	waits := newTestWaits(t)
	seqNo := uint64(324223)
	awaiter, err := waits.NewWaiter(324223)
	assert.NoError(t, err)

	go func() {
		waits.Notify(seqNo) // notify new commit
	}()
	err = waits.Wait(context.TODO(), awaiter)
	assert.NoError(t, err, "wait must return without error")
}

func TestNoDeadlockWhenNoSubscribers(t *testing.T) {
	waits := newTestWaits(t)

	seqNo := uint64(324223)
	_, err := waits.NewWaiter(seqNo)
	assert.NoError(t, err)
	waits.Notify(seqNo) // must be no deadlock here
}

func TestWaitCommitCancelByTimeout(t *testing.T) {
	waits := newTestWaits(t)

	seqNo := uint64(324223)
	awaiter, err := waits.NewWaiter(seqNo)
	assert.NoError(t, err)

	ctx, cancel := context.WithCancel(context.TODO())
	cancel()

	err = waits.Wait(ctx, awaiter)
	assert.Error(t, err, "return by timeout")
}

func TestWaitCommitWithTimeout(t *testing.T) {
	waits := newTestWaits(t)

	seqNo := uint64(324223)
	awaiter, err := waits.NewWaiter(seqNo)
	assert.NoError(t, err)

	err = waits.WaitWithTimeout(context.TODO(), awaiter, 100*time.Millisecond)
	assert.Error(t, err, "return by timeout")
}

func newTestWaits(t *testing.T) *waits {
	logger, err := zap.New(zap.JSONConfig(log.DebugLevel))
	assert.NoError(t, err)
	return newWaits(logger)
}
