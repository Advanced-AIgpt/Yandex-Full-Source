package solomonbatch

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BatchPusher struct {
	solomonSender solomonapi.Sender
	logger        log.Logger
	newMetricsC   chan callbackRequest
	config        Config
	signals       signals
}

func NewBatchPusher(solomonSender solomonapi.Sender, logger log.Logger, registry metrics.Registry, config Config) (*BatchPusher, error) {
	if err := config.Validate(); err != nil {
		return nil, xerrors.Errorf("config validation failed: %w", err)
	}

	return &BatchPusher{
		solomonSender: solomonSender,
		logger:        logger,
		config:        config,
		signals:       newSignals(registry),
		newMetricsC:   make(chan callbackRequest, config.Buffer),
	}, nil
}

func (b *BatchPusher) SendMetrics(ctx context.Context, shard solomonapi.Shard, metrics []solomonapi.Metric) error {
	b.signals.PutInBatchCalls.Add(1)
	callbackChannel := make(chan error, 1)
	request := callbackRequest{
		shard:    shard,
		metrics:  metrics,
		callback: callbackChannel,
	}

	select {
	case b.newMetricsC <- request:
		ctx, cancel := context.WithTimeout(ctx, b.config.CallbackTimeout)
		defer cancel()
		select {
		case err := <-callbackChannel:
			return err
		case <-ctx.Done():
			return xerrors.Errorf("send timeout exceed, metrics must be resent")
		}
	default: // skip sending if buffer is full - prevent OOM in the app
		b.signals.BufferOverflow.Add(1)
		return xerrors.Errorf("batch buffer is full, skip sending metrics")
	}
}

func (b *BatchPusher) Run(ctx context.Context) (err error) {
	b.logger.Infof("started solomon batch background process with interval %s", b.config.SendInterval)
	var batch ShardedMetrics
	for {
		batch, err = b.waitForBatch(ctx)
		if err != nil {
			break
		}

		if err = b.sendBatch(ctx, batch); err != nil {
			ctxlog.Errorf(ctx, b.logger, "failed to send batches to solomon: %v", err)
		}
	}

	if err != nil {
		if xerrors.Is(err, context.Canceled) {
			b.logger.Infof("send last solomon batch for %d shards on graceful shutdown", len(batch))
			lastCtx, cancel := context.WithTimeout(context.Background(), b.config.ShutdownTimeout)
			defer cancel()
			if err = b.sendBatch(lastCtx, batch); err != nil {
				return xerrors.Errorf("failed to send last solomon batch on shutdown: %w", err)
			} else {
				b.logger.Infof("successfully last solomon batch for %d shards on graceful shutdown", len(batch))
				return
			}
		}
	}
	return
}

func (b *BatchPusher) sendBatch(ctx context.Context, batch ShardedMetrics) error {
	if len(batch) == 0 {
		return nil
	}

	defer func(sendTime time.Time) {
		b.signals.BatchTime.RecordDuration(time.Since(sendTime))
	}(time.Now())

	// sending batched requests to solomon
	// each shard must be sent as a separate request
	wg := goroutines.Group{}
	for batchShard, shardData := range batch {
		shard, shardMetrics, shardCallbacks := batchShard, shardData.metrics, shardData.callbacks
		wg.Go(func() error {
			b.logger.Infof("push %d metrics to shard with service %s", len(shardMetrics), shard.Service)
			err := b.solomonSender.SendMetrics(ctx, shard, shardMetrics)
			if err != nil {
				b.signals.ErrorBatchSend.Add(1)
			} else {
				b.signals.SuccessBatchSend.Add(1)
			}

			for _, callback := range shardCallbacks { // send callbacks to goroutines
				select {
				case callback <- err:
				default:
					b.logger.Errorf("failed to send callback for shard %s", shard.Service)
				}
				close(callback)
			}
			return err
		})
	}
	return wg.Wait()
}

func (b *BatchPusher) waitForBatch(ctx context.Context) (ShardedMetrics, error) {
	batch := make(ShardedMetrics)
	timer := time.NewTimer(b.config.SendInterval)
	defer timer.Stop()

	for {
		select {
		case newRequest, ok := <-b.newMetricsC:
			if !ok {
				return nil, xerrors.Errorf("new metrics channel has been closed")
			}
			b.signals.UsedBuffer.Set(float64(len(b.newMetricsC)))
			batch[newRequest.shard] = metricsWithCallbacks{
				metrics:   append(batch[newRequest.shard].metrics, newRequest.metrics...),
				callbacks: append(batch[newRequest.shard].callbacks, newRequest.callback),
			}

			if len(batch[newRequest.shard].metrics) > int(b.config.Limit) {
				b.logger.Warnf("requests in shard %s exceed batch limit %d", newRequest.shard, b.config.Limit)
				return batch, nil
			}
		case <-timer.C:
			return batch, nil
		case <-ctx.Done():
			return batch, xerrors.Errorf("context done %w", ctx.Err())
		}
	}
}
