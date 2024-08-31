package logbroker

import (
	"context"
	"fmt"
	"sync"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Waiter waits message ack to logbroker
type Waiter interface {
	SeqNo() uint64 // message uniq number
	WaitC() <-chan WaitResult
}

// waits manages all waits for ack from logbroker
type waits struct {
	waitAcks map[uint64]ackWaiter
	mu       *sync.RWMutex
	logger   log.Logger
}

func newWaits(logger log.Logger) *waits {
	return &waits{
		waitAcks: make(map[uint64]ackWaiter),
		mu:       &sync.RWMutex{},
		logger:   logger,
	}
}

// NewWaiter creates new waiter for commit with given seqNo
func (w *waits) NewWaiter(seqNo uint64) (Waiter, error) {
	w.mu.Lock()
	defer w.mu.Unlock()
	if _, ok := w.waitAcks[seqNo]; ok {
		return nil, fmt.Errorf("failed to add wait ack %d because it already exists", seqNo)
	}

	notifyC := make(chan WaitResult, 1) // buffered channel insures to avoid deadlock if waiter decided not to wait commit
	w.waitAcks[seqNo] = ackWaiter{
		notifyC: notifyC,
	}

	return &awaiter{
		seqNo:   seqNo,
		notifyC: notifyC,
	}, nil
}

// Notify sends message to waiter that write was acked from logbroker
func (w *waits) Notify(seqNo uint64) {
	w.NotifyWithError(seqNo, nil)
}

// NotifyWithError sends message to waiter that write was acked from logbroker or received error
func (w *waits) NotifyWithError(seqNo uint64, err error) {
	w.mu.Lock()
	if waitAck, ok := w.waitAcks[seqNo]; ok {
		waitAck.notifyC <- WaitResult{Err: err}
		close(waitAck.notifyC)
		delete(w.waitAcks, seqNo)
		w.logger.Tracef("notify ack waiter for seqNo %d, err: %v", seqNo, err)
	}
	w.mu.Unlock()
}

func (w *waits) Delete(seqNo uint64) {
	w.mu.Lock()
	if waitAck, ok := w.waitAcks[seqNo]; ok {
		close(waitAck.notifyC)
		delete(w.waitAcks, seqNo)
	}
	w.mu.Unlock()
}

// WaitWithTimeout waits for given message ack from logbroker
func (w *waits) WaitWithTimeout(ctx context.Context, waiter Waiter, timeout time.Duration) error {
	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()
	return w.Wait(ctx, waiter)
}

// Wait waits for given message ack from logbroker
func (w *waits) Wait(ctx context.Context, waiter Waiter) error {
	defer w.Delete(waiter.SeqNo()) // clean up waiter
	select {
	case result, ok := <-waiter.WaitC():
		if !ok {
			return xerrors.Errorf("failed to wait message commit")
		}

		if result.Err != nil {
			return xerrors.Errorf("failed to commit message: %w", result.Err)
		}

		return nil
	case <-ctx.Done():
		return xerrors.Errorf("message wait cancelled by context")
	}
}

// ackWaiter is data stored for each waiter
type ackWaiter struct {
	notifyC chan WaitResult // channel for notifying message was acked
}

// awaiter - wait-object which allows wait logbroker writer ack
type awaiter struct {
	seqNo   uint64
	notifyC <-chan WaitResult
}

func (w *awaiter) SeqNo() uint64 {
	return w.seqNo
}

func (w *awaiter) WaitC() <-chan WaitResult {
	return w.notifyC
}

type WaitResult struct {
	Err error
}
