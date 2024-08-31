package solomonapi

import "context"

type Sender interface {
	// SendMetrics pushes solomon metrics to solomon api
	// see https://docs.yandex-team.ru/solomon/data-collection/push#push-v-solomon-api-%7Bpush-api%7D
	SendMetrics(ctx context.Context, shard Shard, metrics []Metric) error
}

type Fetcher interface {
	// FetchData loads metrics from solomon
	// see https://solomon.yandex-team.ru/swagger-ui/index.html#/data
	FetchData(ctx context.Context, project string, dataRequest DataRequest) (*DataResponse, error)
}

type Mock struct{}

func NewMock() Mock {
	return Mock{}
}

func (m Mock) FetchData(ctx context.Context, project string, dataRequest DataRequest) (*DataResponse, error) {
	return nil, nil
}

func (m Mock) SendMetrics(ctx context.Context, shard Shard, metrics []Metric) error {
	return nil
}
