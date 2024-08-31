package logbroker

import (
	"context"
	"golang.org/x/sync/errgroup"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type reader struct {
	logger                 log.Logger
	reader                 persqueue.Reader
	metrics                readerMetrics
	batchHandler           func(ctx context.Context, batch persqueue.MessageBatch) error
	collectMetricsInterval time.Duration
}

type Reader interface {
	// Start runs reader for receiving messages from logbroker
	Start(ctx context.Context) error
}

type ReaderOptions struct {
	CollectMetricsInterval time.Duration
	Logbroker              persqueue.ReaderOptions
}

func NewReader(
	logger log.Logger,
	metrics metrics.Registry,
	options ReaderOptions,
	batchHandler func(ctx context.Context, batch persqueue.MessageBatch) error,
) *reader {
	readerMetrics := readerMetrics{
		MemUsage:         metrics.Gauge("mem_usage"),
		BytesExtracted:   metrics.Gauge("bytes_extracted"),
		BytesRead:        metrics.Gauge("bytes_read"),
		CompressionRatio: metrics.Gauge("compression_ratio"),
		InflightCount:    metrics.Gauge("inflight_count"),
		WaitForAck:       metrics.Gauge("wait_for_ack"),
	}
	solomon.Rated(readerMetrics.BytesRead)
	solomon.Rated(readerMetrics.BytesExtracted)

	return &reader{
		logger:                 logger,
		reader:                 persqueue.NewReader(options.Logbroker),
		batchHandler:           batchHandler,
		metrics:                readerMetrics,
		collectMetricsInterval: options.CollectMetricsInterval,
	}
}

func (r *reader) Start(ctx context.Context) error {
	readerInit, err := r.reader.Start(ctx)
	if err != nil {
		return xerrors.Errorf("failed to start reading from logbroker: %w", err)
	}
	ctxlog.Infof(ctx, r.logger, "start logbroker reading session with id: %s", readerInit.SessionID)

	wg, ctx := errgroup.WithContext(ctx)

	wg.Go(func() error {
		r.serveReadBatches(ctx)
		return nil
	})

	wg.Go(func() error {
		r.serveMetricsCollection(ctx)
		return nil
	})

	if err := wg.Wait(); err != nil { // error must never happend
		ctxlog.Errorf(ctx, r.logger, "error on serving logbroker reader: %v", err)
	}

	ctxlog.Infof(ctx, r.logger, "shutdown logbroker reader")
	r.reader.Shutdown()
	<-r.reader.Closed() // wait till the reader closed
	if err = r.reader.Err(); err != nil {
		ctxlog.Errorf(ctx, r.logger, "logbroker reader stopped: %v", err)
		return err
	}
	return nil
}

func (r *reader) serveReadBatches(ctx context.Context) {
	// read all events from logbroker
	for event := range r.reader.C() {
		switch e := event.(type) {
		case *persqueue.Data:
			wg, ctx := errgroup.WithContext(ctx)
			for _, batch := range e.Batches() {
				processedBatch := batch
				// process each batch from different topics in parallel
				wg.Go(func() error {
					return r.batchHandler(ctx, processedBatch)
				})
			}
			if err := wg.Wait(); err != nil {
				ctxlog.Errorf(ctx, r.logger, "error during logbroker batch processing: %v", err)
			}
			e.Commit()
			r.metrics.UpdateMetrics(r.reader.Stat())
		case *persqueue.CommitAck:
			// do nothing
		case *persqueue.Lock:
			// logbroker offers a new partition
			ctxlog.Infof(ctx, r.logger, "logbroker offers a new partition %d for topic %s [read_offset: %d, end_offset: %d, generation: %d]",
				e.Partition, e.Topic, e.ReadOffset, e.EndOffset, e.Generation)
		case *persqueue.LockV1:
			// logbroker offers a new partition
			ctxlog.Infof(ctx, r.logger, "logbroker offers a new partition %d for topic %s [read_offset: %d, end_offset: %d]",
				e.Partition, e.Topic, e.ReadOffset, e.EndOffset)
		case *persqueue.Release:
			// logbroker take partition away and no more data and commits
			ctxlog.Infof(ctx, r.logger, "logbroker take away partition %d for topic %s", e.Partition, e.Topic)
		case *persqueue.ReleaseV1:
			// logbroker take partition away and no more data and commits
			ctxlog.Infof(ctx, r.logger, "logbroker take away partition %d for topic %s", e.Partition, e.Topic)
		case *persqueue.Disconnect:
			// disconnect is message that is emitted by reader when connection to the server is lost
			ctxlog.Infof(ctx, r.logger, "logbroker reader has been disconnected: %v", e.Err)
		default:
			ctxlog.Warnf(ctx, r.logger, "unknown event from logbroker reader: %v", e)
		}
	}
}

func (r *reader) serveMetricsCollection(ctx context.Context) {
	ctxlog.Infof(ctx, r.logger, "start logbroker reader metrics collection with interval: %v", r.collectMetricsInterval)
	ticker := time.NewTicker(r.collectMetricsInterval)
	defer ticker.Stop()
	for {
		select {
		case <-ticker.C:
			stat := r.reader.Stat()
			r.metrics.UpdateMetrics(stat)
		case <-ctx.Done():
			return
		}
	}
}

type readerMetrics struct {
	MemUsage         metrics.Gauge
	BytesExtracted   metrics.Gauge
	BytesRead        metrics.Gauge
	InflightCount    metrics.Gauge
	WaitForAck       metrics.Gauge
	CompressionRatio metrics.Gauge
}

func (r *readerMetrics) UpdateMetrics(stat persqueue.Stat) {
	r.MemUsage.Set(float64(stat.MemUsage))
	r.BytesExtracted.Set(float64(stat.BytesExtracted))
	r.BytesRead.Set(float64(stat.BytesRead))
	r.InflightCount.Set(float64(stat.InflightCount))
	r.WaitForAck.Set(float64(stat.WaitAckCount))

	if stat.BytesRead > 0 {
		r.CompressionRatio.Set(float64(stat.BytesExtracted) / float64(stat.BytesRead))
	}
}
