package solomonbatch

import (
	"fmt"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Limit           uint
	SendInterval    time.Duration
	Buffer          uint
	CallbackTimeout time.Duration
	ShutdownTimeout time.Duration
}

func (c Config) Validate() error {
	if c.CallbackTimeout < c.SendInterval {
		return xerrors.New(fmt.Sprintf("CallbackTimeout %v can't be less then SendInterval %v", c.CallbackTimeout, c.SendInterval))
	}
	return nil
}

type signals struct {
	SuccessBatchSend  metrics.Counter
	ErrorBatchSend    metrics.Counter
	PutInBatchCalls   metrics.Counter
	BufferOverflow    metrics.Counter
	SendErrors        metrics.Counter
	SendTimeoutErrors metrics.Counter
	UsedBuffer        metrics.Gauge
	BatchTime         metrics.Timer
}

func newSignals(registry metrics.Registry) signals {
	batchSignals := signals{
		SuccessBatchSend:  registry.Counter("success_batch_send"),
		ErrorBatchSend:    registry.Counter("error_batch_send"),
		PutInBatchCalls:   registry.Counter("put_in_batch_calls"),
		BufferOverflow:    registry.Counter("buffer_overflow"),
		UsedBuffer:        registry.Gauge("used_buffer"),
		SendErrors:        registry.Counter("send_errors"),
		SendTimeoutErrors: registry.Counter("send_timeout_errors"),
		BatchTime:         registry.DurationHistogram("batch_time", quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
	solomon.Rated(batchSignals.SuccessBatchSend)
	solomon.Rated(batchSignals.ErrorBatchSend)
	solomon.Rated(batchSignals.PutInBatchCalls)
	solomon.Rated(batchSignals.BufferOverflow)
	solomon.Rated(batchSignals.SendErrors)
	solomon.Rated(batchSignals.SendTimeoutErrors)
	return batchSignals
}

type callbackRequest struct {
	callback chan<- error
	shard    solomonapi.Shard
	metrics  []solomonapi.Metric
}

type metricsWithCallbacks struct {
	metrics   []solomonapi.Metric
	callbacks []chan<- error
}

type ShardedMetrics map[solomonapi.Shard]metricsWithCallbacks
