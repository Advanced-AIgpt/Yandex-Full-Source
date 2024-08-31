package logbroker

import (
	"context"
	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"golang.org/x/sync/errgroup"
	"os"
	"sort"
	"strconv"
	"testing"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type LBConfig struct {
	Database     string
	Port         int
	Endpoint     string
	Topic        string
	ConsumerName string
}

type TestSuite struct {
	suite.Suite
	LogbrokerConfig LBConfig
	Logger          log.Logger
	BatchC          chan persqueue.MessageBatch
	Registry        metrics.Registry
}

func (s *TestSuite) SetupTest() {
	logger, err := zap.New(zap.JSONConfig(log.DebugLevel))
	assert.NoError(s.T(), err)
	s.Logger = logger
	s.BatchC = make(chan persqueue.MessageBatch, 50)
	s.Registry = solomon.NewRegistry(nil)

	lbPort, err := strconv.Atoi(os.Getenv("LOGBROKER_PORT"))
	assert.NoError(s.T(), err)
	s.LogbrokerConfig = LBConfig{
		Database:     "/Root",
		Port:         lbPort,
		Endpoint:     "127.0.0.1",
		Topic:        "default-topic",
		ConsumerName: "default-consumer",
	}
}

func (s *TestSuite) newWriter() Writer {
	sessionUUID, err := uuid.NewV4()
	assert.NoError(s.T(), err)

	return NewWriter(
		s.Logger,
		s.Registry,
		WriterOptions{
			AckTimeout:             500 * time.Millisecond,
			CollectMetricsInterval: 3 * time.Second,
			Logbroker: persqueue.WriterOptions{
				Database:       s.LogbrokerConfig.Database,
				Endpoint:       s.LogbrokerConfig.Endpoint,
				Port:           s.LogbrokerConfig.Port,
				Logger:         corelogadapter.New(s.Logger),
				RetryOnFailure: false,
				Topic:          s.LogbrokerConfig.Topic,
				SourceID:       sessionUUID.Bytes(),
			},
		})
}

func (s *TestSuite) newWritePool(partitionCount uint32) WritePool {
	partitions, err := GenerateSequenceWithUUIDS(partitionCount)
	assert.NoError(s.T(), err)
	pool, err := NewWritePool(s.Logger,
		s.Registry,
		WritePoolOptions{
			AckTimeout:             500 * time.Millisecond,
			CollectMetricsInterval: 3 * time.Second,
			Partitions:             partitions,
			TemplateOptions: persqueue.WriterOptions{
				Database:       s.LogbrokerConfig.Database,
				Endpoint:       s.LogbrokerConfig.Endpoint,
				Port:           s.LogbrokerConfig.Port,
				Logger:         corelogadapter.New(s.Logger),
				RetryOnFailure: false,
				Topic:          s.LogbrokerConfig.Topic,
			},
		},
	)
	assert.NoError(s.T(), err)
	return pool
}

func (s *TestSuite) newReader(consumer string) *reader {
	return NewReader(s.Logger,
		s.Registry,
		ReaderOptions{
			CollectMetricsInterval: 1 * time.Second,
			Logbroker: persqueue.ReaderOptions{
				Database: s.LogbrokerConfig.Database,
				Endpoint: s.LogbrokerConfig.Endpoint,
				Port:     s.LogbrokerConfig.Port,
				Logger:   corelogadapter.New(s.Logger),
				Consumer: consumer,
				Topics:   []persqueue.TopicInfo{{Topic: s.LogbrokerConfig.Topic}},
			},
		}, func(ctx context.Context, batch persqueue.MessageBatch) error {
			s.BatchC <- batch
			return nil
		})
}

func (s *TestSuite) TestWriterAsyncMessagesAndRead() {
	s.LogbrokerConfig.Topic = "writer-topic" // specified in gotest/ya.make

	t := s.T()
	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, 5*time.Second)
	defer cancel()

	writer := s.newWriter()
	err := writer.Init(ctx)
	assert.NoError(t, err)
	lbReader := s.newReader(s.LogbrokerConfig.ConsumerName)

	wg, ctx := errgroup.WithContext(ctx)
	wg.Go(func() error {
		return writer.Serve(ctx)
	})

	wg.Go(func() error {
		return lbReader.Start(ctx)
	})

	sendMessages := []string{
		"message 1",
		"message 2",
		"message 3",
		"message 4",
	}

	// send messages async
	for _, msg := range sendMessages {
		go func(sendMsg string) {
			err := writer.WriteWithAck(ctx, []byte(sendMsg))
			assert.NoError(t, err)
		}(msg)
	}

	readMessages, err := s.waitReadTopicMessages(ctx, len(sendMessages)) // wait for reading all messages or ctx cancel
	assert.NoError(t, err)
	cancel()
	_ = wg.Wait()

	sort.Strings(readMessages) // async message can be any order
	assert.Equal(t, sendMessages, readMessages)
}

func (s *TestSuite) TestWriterPoolAsyncMessagesAndRead() {
	s.LogbrokerConfig.Topic = "pool-topic" // specified in gotest/ya.make
	t := s.T()
	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, 5*time.Second)
	defer cancel()

	partitionCount := uint32(2)
	writePool := s.newWritePool(partitionCount)
	err := writePool.Init(ctx)
	assert.NoError(t, err)
	lbReader := s.newReader(s.LogbrokerConfig.ConsumerName)

	wg, ctx := errgroup.WithContext(ctx)
	wg.Go(func() error {
		return writePool.Serve(ctx)
	})

	wg.Go(func() error {
		return lbReader.Start(ctx)
	})

	sendMessages := []string{
		"message 1",
		"message 2",
		"message 3",
		"message 4",
		"message 5",
		"message 6",
	}

	// send messages async in different partitions
	for i, msg := range sendMessages {
		part := uint32(0)
		if i%2 > 0 { //split messages by two partitions
			part = 1
		}

		go func(sendMsg string, partition uint32) {
			err := writePool.WriteWithAck(ctx, partition, []byte(sendMsg))
			assert.NoError(t, err)
		}(msg, part)
	}

	readMessages, err := s.waitReadTopicMessages(ctx, len(sendMessages)) // wait for reading all messages or ctx cancel
	assert.NoError(t, err)
	cancel()
	_ = wg.Wait()

	sort.Strings(readMessages) // async message can be any order
	assert.Equal(t, sendMessages, readMessages)
}

func (s *TestSuite) waitReadTopicMessages(ctx context.Context, readCount int) ([]string, error) {
	messages := make([]string, 0)
	for {
		select {
		case batch := <-s.BatchC:
			for _, msg := range batch.Messages {
				messages = append(messages, string(msg.Data))
				if len(messages) == readCount {
					return messages, nil
				}
			}
		case <-ctx.Done():
			return nil, ctx.Err()
		}
	}
}

func (s *TestSuite) TearDownTest() {
	close(s.BatchC)
}

func TestLogbroker(t *testing.T) {
	suite.Run(t, new(TestSuite))
}
