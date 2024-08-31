package history

import (
	"context"
	"fmt"
	"sort"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	sensorLabel = "sensor"
)

type SolomonService string

type SolomonShard struct {
	Project       string
	ServicePrefix string
	Cluster       string
}

func (s SolomonShard) GetServiceName(propertyInstance model.PropertyInstance) SolomonService {
	// there is a cluster for each float property (humidity, temperature, etc)
	// we use a different solomon shard for each property - for scaling reason
	// solomon shard is a unique combination (service, cluster)
	return SolomonService(fmt.Sprintf("%s-%s", s.ServicePrefix, propertyInstance.String()))
}

// PushMetricsToSolomon sends metrics directly to solomon. Its used only for load testing purposes
func (c *Controller) PushMetricsToSolomon(ctx context.Context, deviceProperties model.DevicePropertiesMap) error {
	return c.pushMetricsToSolomon(ctx, deviceProperties)
}

func (c *Controller) pushMetricsToSolomon(ctx context.Context, deviceProperties model.DevicePropertiesMap) error {
	shardedMetrics := mapDevicePropertiesToMetrics(c.solomonShard, deviceProperties)
	// in the most cases its only one shard in deviceProperties
	for serviceName, metrics := range shardedMetrics {
		shard := solomonapi.Shard{
			Project: c.solomonShard.Project,
			Service: string(serviceName),
			Cluster: c.solomonShard.Cluster,
		}
		if err := c.solomonSender.SendMetrics(ctx, shard, metrics); err != nil {
			return xerrors.Errorf("failed to push metrics to solomon: %w", err)
		}
	}
	return nil
}

func mapDevicePropertiesToMetrics(solomonShard SolomonShard, deviceProperties model.DevicePropertiesMap) map[SolomonService][]solomonapi.Metric {
	shardedMetrics := make(map[SolomonService][]solomonapi.Metric)
	for deviceID, properties := range deviceProperties {
		for _, property := range properties {
			if property.State() == nil {
				continue
			}
			switch property.Type() {
			case model.FloatPropertyType:
				state := property.State().(model.FloatPropertyState)
				clusterName := solomonShard.GetServiceName(state.Instance)
				shardedMetrics[clusterName] = append(shardedMetrics[clusterName], solomonapi.Metric{
					Labels: solomonapi.Labels{
						sensorLabel: deviceID,
					},
					Value:     state.Value,
					Timestamp: property.LastUpdated().AsTime(),
				})
			}
		}
	}
	return shardedMetrics
}

func solomonRequestForFloatProperty(shard SolomonShard, deviceID string, instance model.PropertyInstance) string {
	return fmt.Sprintf(`{project="%s", cluster="%s", service="%s", %s="%s"}`,
		shard.Project,
		shard.Cluster,
		shard.GetServiceName(instance),
		sensorLabel,
		deviceID,
	)
}

type MetricAggregation map[solomonapi.Aggregation]float64

type MetricValue struct {
	Time  time.Time
	Value MetricAggregation
}

type DeviceHistoryRequest struct {
	DeviceID     string
	Instance     model.PropertyInstance
	From         time.Time
	To           time.Time
	Grid         time.Duration
	Aggregations []solomonapi.Aggregation
	GapFilling   solomonapi.GapFillingType
}

// apiResponse is wrapper for helping eliminate concurrency during parallel fetching
type apiResponse struct {
	data *solomonapi.DataResponse
}

func (c *Controller) FetchAggregatedDeviceHistory(ctx context.Context, request DeviceHistoryRequest) ([]MetricValue, error) {
	group := goroutines.Group{}
	parallelResult := make(map[solomonapi.Aggregation]*apiResponse)
	for _, aggregation := range request.Aggregations {
		aggregationType, response := aggregation, &apiResponse{}
		parallelResult[aggregationType] = response

		group.Go(func() (err error) {
			// apiResponse required as wrapper to eliminate concurrency sync in rawResult
			response.data, err = c.solomonFetcher.FetchData(ctx, c.solomonShard.Project, solomonapi.DataRequest{
				Program: solomonRequestForFloatProperty(c.solomonShard, request.DeviceID, request.Instance),
				Downsampling: solomonapi.DataDownsampling{
					Fill:        request.GapFilling,
					Aggregation: aggregationType,
					GridMillis:  uint64(request.Grid.Milliseconds()),
				},
				FromMilli: request.From.UnixMilli(),
				ToMilli:   request.To.UnixMilli(),
			})
			if err != nil {
				err = xerrors.Errorf("failed to load metrics with aggregation %s: %w", aggregationType, err)
			}
			return
		})
	}

	err := group.Wait()
	if err != nil {
		return nil, xerrors.Errorf("failed to load history metrics: %w", err)
	}

	return mapRawSolomonResponseToMetrics(parallelResult)
}

func mapRawSolomonResponseToMetrics(rawResult map[solomonapi.Aggregation]*apiResponse) ([]MetricValue, error) {
	// first, group all aggregates value by timestamps
	timestampAggregates := make(map[int64]MetricAggregation)
	for aggregationType, solomonResponse := range rawResult {
		if solomonResponse == nil || solomonResponse.data == nil {
			return nil, xerrors.Errorf("history response can't be nil for aggregation %s", aggregationType)
		}

		if len(solomonResponse.data.Vector) == 0 {
			continue
		}

		timeseries := solomonResponse.data.Vector[0].Timeseries
		if len(timeseries.Timestamps) != len(timeseries.Values) {
			return nil, xerrors.Errorf("timestamps (len %d) and values (len %d) must have the same length for aggregation: %s",
				len(timeseries.Timestamps), len(timeseries.Values), aggregationType)
		}
		for i := 0; i < len(timeseries.Timestamps); i++ {
			ts := timeseries.Timestamps[i]
			if _, ok := timestampAggregates[ts]; !ok {
				timestampAggregates[ts] = MetricAggregation{}
			}
			if timeseries.Values[i].Val != nil {
				timestampAggregates[ts][aggregationType] = *timeseries.Values[i].Val
			}
		}
	}

	metrics := make([]MetricValue, 0, len(timestampAggregates))
	for ts, val := range timestampAggregates {
		value := val
		if len(value) < len(rawResult) { // when part of aggregations are nil
			value = nil
		}

		metrics = append(metrics, MetricValue{
			Time:  time.UnixMilli(ts),
			Value: value,
		})
	}

	// return metrics sorted ascending by timestamp
	sort.SliceStable(metrics, func(i, j int) bool {
		return metrics[i].Time.Before(metrics[j].Time)
	})
	return metrics, nil
}
