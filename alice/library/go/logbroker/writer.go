package logbroker

import (
	"context"
	"golang.org/x/sync/errgroup"
	"sync"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Writer interface {
	// Init initializes writer instance with connection to logbroker
	Init(ctx context.Context) error
	// Serve starts serving callbacks on writes from logbroker
	Serve(ctx context.Context) error
	// WriteWithAck sends async message to logbroker and waits for ack for successful write from logbroker server
	WriteWithAck(ctx context.Context, data []byte) error
}

type WriterOptions struct {
	AckTimeout             time.Duration // timeout for waiting successful write confirmation
	CollectMetricsInterval time.Duration // how often collect metrics
	Logbroker              persqueue.WriterOptions
}

type writer struct {
	logger         log.Logger
	writer         persqueue.Writer
	sequenceNumber uint64
	partition      uint64
	ackWaits       *waits
	writeMx        *sync.RWMutex

	ackTimeout             time.Duration
	collectMetricsInterval time.Duration
	metrics                writerMetrics
}

func NewWriter(
	logger log.Logger,
	registry metrics.Registry,
	options WriterOptions,
) Writer {
	return &writer{
		logger:                 logger,
		writer:                 persqueue.NewWriter(options.Logbroker),
		sequenceNumber:         0,
		partition:              0,
		writeMx:                &sync.RWMutex{},
		ackWaits:               newWaits(logger),
		ackTimeout:             options.AckTimeout,
		collectMetricsInterval: options.CollectMetricsInterval,
		metrics: writerMetrics{
			Inflight: registry.Gauge("inflight"),
			MemUsage: registry.Gauge("mem_usage"),
		},
	}
}

func (w *writer) Init(ctx context.Context) error {
	init, err := w.writer.Init(ctx)
	if err != nil {
		return xerrors.Errorf("failed to init logbroker writer: %w", err)
	}

	w.writeMx.Lock()
	w.sequenceNumber = init.MaxSeqNo
	w.partition = init.Partition
	w.writeMx.Unlock()

	ctxlog.Infof(ctx, w.logger, "start writer [cluster: %s, session_id: %s, topic: %s, partition: %d, maxSeqNo: %d]",
		init.Cluster, init.SessionID, init.Topic, init.Partition, init.MaxSeqNo)

	return nil
}

func (w *writer) Serve(ctx context.Context) error {
	ctxlog.Infof(ctx, w.logger, "start writer serving")

	defer func() {
		if err := w.writer.Close(); err != nil {
			ctxlog.Errorf(ctx, w.logger, "error during closing logbroker writer: %v", err)
		}
	}()

	wg, ctx := errgroup.WithContext(ctx)
	wg.Go(func() error {
		return w.processWriterResponses(ctx)
	})

	wg.Go(func() error {
		return w.collectMetrics(ctx)
	})

	return wg.Wait()
}

func (w *writer) processWriterResponses(ctx context.Context) error {
	for r := range w.writer.C() {
		switch rsp := r.(type) {
		case *persqueue.Ack:
			w.ackWaits.Notify(rsp.SeqNo) // notify waiter message was accepted by logbroker
		case *persqueue.Issue:
			ctxlog.Warnf(ctx, w.logger, "issue on writer response for message: %v, %v", rsp.Err, rsp.Data)
			if rsp.Data != nil {
				// notify ack waits with error if we know message
				w.ackWaits.NotifyWithError(rsp.Data.GetAssignedSeqNo(), rsp.Err)
			}
		}
	}
	return nil
}

func (w *writer) collectMetrics(ctx context.Context) error {
	ticker := time.NewTicker(w.collectMetricsInterval)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			stat := w.writer.Stat()
			w.metrics.MemUsage.Set(float64(stat.MemUsage))
			w.metrics.Inflight.Set(float64(stat.Inflight))
		case <-ctx.Done():
			return nil
		}
	}
}

func (w *writer) WriteWithAck(ctx context.Context, data []byte) error {
	waiter, err := w.sendWrite(data)
	if err != nil {
		return xerrors.Errorf("failed to send write to logbroker: %w", err)
	}
	return w.ackWaits.WaitWithTimeout(ctx, waiter, w.ackTimeout)
}

func (w *writer) sendWrite(data []byte) (Waiter, error) {
	w.writeMx.Lock()
	defer w.writeMx.Unlock()

	msg := &persqueue.WriteMessage{Data: data}
	w.sequenceNumber += 1 // sequence number must grow monotonically

	seqNo := w.sequenceNumber
	msg.WithSeqNo(w.sequenceNumber)
	waiter, err := w.ackWaits.NewWaiter(seqNo) // create waiter to be waiting message ack from logbroker
	if err != nil {
		return nil, xerrors.Errorf("failed to create ack waiter for logbroker commit: %w", err)
	}
	if err := w.writer.Write(msg); err != nil {
		return nil, xerrors.Errorf("failed to write msg: %w", err)
	}
	return waiter, nil
}

type writerMetrics struct {
	Inflight metrics.Gauge // number of current inflight messages
	MemUsage metrics.Gauge // mem usage in bytes
}
