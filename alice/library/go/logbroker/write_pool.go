package logbroker

import (
	"context"
	"github.com/gofrs/uuid"
	"golang.org/x/sync/errgroup"
	"strconv"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type WritePool interface {
	Init(ctx context.Context) error
	Serve(ctx context.Context) error
	WriteWithAck(ctx context.Context, partition uint32, data []byte) error
}

type WritePoolOptions struct {
	// base options for logbroker writer
	// Partition and SessionID will be overridden from Partitions
	TemplateOptions persqueue.WriterOptions
	// how many writers with given partition number and session id must be started
	Partitions []WritePoolPartition

	AckTimeout             time.Duration // timeout for successful write to the server
	CollectMetricsInterval time.Duration // how often collect solomon metrics
}

// WritePoolPartition describes writer partition number and source id (session-id)
type WritePoolPartition struct {
	Number   uint32 // partition number (starts from 0)
	SourceID []byte // aka sessionID, aka message group id
}

// GenerateSequenceWithUUIDS generates sequence of partitions [0, partitionCount) with random sourceID
func GenerateSequenceWithUUIDS(partitionCount uint32) ([]WritePoolPartition, error) {
	partitions := make([]WritePoolPartition, 0, partitionCount)
	for partition := uint32(0); partition < partitionCount; partition++ {
		sessionUUID, err := uuid.NewV4()
		if err != nil {
			return nil, xerrors.Errorf("failed to generate uuid: %w", err)
		}
		partitions = append(partitions, WritePoolPartition{
			Number:   partition,
			SourceID: sessionUUID.Bytes(),
		})
	}
	return partitions, nil
}

type writerPool struct {
	logger             log.Logger
	writersByPartition map[uint32]Writer
}

func NewWritePool(
	logger log.Logger,
	registry metrics.Registry,
	options WritePoolOptions,
) (WritePool, error) {
	writersByPartition := make(map[uint32]Writer)
	for _, partition := range options.Partitions {
		partitionOptions := options.TemplateOptions
		partitionOptions.PartitionGroup = partition.Number + 1
		partitionOptions.SourceID = partition.SourceID

		partitionMetrics := registry.WithTags(map[string]string{
			"topic":     partitionOptions.Topic,
			"partition": strconv.Itoa(int(partition.Number)),
		})
		writersByPartition[partition.Number] = NewWriter(logger, partitionMetrics, WriterOptions{
			AckTimeout:             options.AckTimeout,
			CollectMetricsInterval: options.CollectMetricsInterval,
			Logbroker:              partitionOptions,
		})
	}

	return &writerPool{
		logger:             logger,
		writersByPartition: writersByPartition,
	}, nil
}

func (w *writerPool) Init(ctx context.Context) error {
	wg := errgroup.Group{} // important to use wg without ctx - otherwise writers will be stopped after leaving the method
	for partition, partitionWriter := range w.writersByPartition {
		currentPartition, currentWriter := partition, partitionWriter
		wg.Go(func() error {
			if err := currentWriter.Init(ctx); err != nil { // use original ctx without cancelling
				return xerrors.Errorf("failed to init logbroker writer for partition %d: %w", currentPartition, err)
			}
			return nil
		})
	}

	// wait to initialize all writers
	if err := wg.Wait(); err != nil {
		return xerrors.Errorf("failed to init worker pool: %w", err)
	}
	return nil
}

func (w *writerPool) Serve(ctx context.Context) error {
	// serve all writers
	wg, ctx := errgroup.WithContext(ctx)
	for partition, partitionWriter := range w.writersByPartition {
		currentPartition, currentWriter := partition, partitionWriter
		wg.Go(func() error {
			if err := currentWriter.Serve(ctx); err != nil {
				return xerrors.Errorf("failed to serve writer for partition %d: %w", currentPartition, err)
			}
			return nil
		})
	}

	return wg.Wait()
}

func (w *writerPool) WriteWithAck(ctx context.Context, partition uint32, data []byte) error {
	writer, ok := w.writersByPartition[partition]
	if !ok {
		return xerrors.Errorf("unknown partition %d in writer pool", partition)
	}
	return writer.WriteWithAck(ctx, data)
}
