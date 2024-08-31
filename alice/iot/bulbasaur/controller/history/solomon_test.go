package history

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func TestMapDevicePropertiesToSolomon(t *testing.T) {
	testCases := []struct {
		name        string
		deviceProps model.DevicePropertiesMap
		expected    map[SolomonService][]solomonapi.Metric
	}{
		{
			name:        "nil props",
			deviceProps: nil,
			expected:    map[SolomonService][]solomonapi.Metric{},
		},
		{
			name:        "empty props",
			deviceProps: model.DevicePropertiesMap{},
			expected:    map[SolomonService][]solomonapi.Metric{},
		},
		{
			name: "map device with several prop",
			deviceProps: model.DevicePropertiesMap{
				"d1": model.Properties{
					makeFloatProperty(1642664985.0, model.FloatPropertyState{
						Instance: "humidity",
						Value:    34.53742,
					}),
					makeFloatProperty(1642664986.332, model.FloatPropertyState{
						Instance: "temperature",
						Value:    21.5,
					}),
				},
				"d2": model.Properties{
					makeFloatProperty(1642664988.442, model.FloatPropertyState{
						Instance: "temperature",
						Value:    18.5,
					}),
				},
			},
			expected: map[SolomonService][]solomonapi.Metric{
				"float-property-humidity": {
					{
						Labels: solomonapi.Labels{
							"sensor": "d1",
						},
						Value:     34.53742,
						Timestamp: timestamp.PastTimestamp(1642664985.0).AsTime(),
					},
				},
				"float-property-temperature": {
					{
						Labels: solomonapi.Labels{
							"sensor": "d1",
						},
						Value:     21.5,
						Timestamp: timestamp.PastTimestamp(1642664986.332).AsTime(),
					},
					{
						Labels: solomonapi.Labels{
							"sensor": "d2",
						},
						Value:     18.5,
						Timestamp: timestamp.PastTimestamp(1642664988.442).AsTime(),
					},
				},
			},
		},
		{
			name: "map device with float and event prop",
			deviceProps: model.DevicePropertiesMap{
				"d1": model.Properties{
					makeFloatProperty(1642664988.442, model.FloatPropertyState{
						Instance: "temperature",
						Value:    18.5,
					}),
					makeEventProperty(1642664984.325, model.EventPropertyState{
						Instance: "open",
						Value:    "on",
					}),
				},
				"d2": model.Properties{
					makeEventProperty(1642664985.442, model.EventPropertyState{
						Instance: "vibration",
						Value:    "on",
					}),
				},
			},
			expected: map[SolomonService][]solomonapi.Metric{
				"float-property-temperature": {
					{
						Labels: solomonapi.Labels{
							"sensor": "d1",
						},
						Value:     18.5,
						Timestamp: timestamp.PastTimestamp(1642664988.442).AsTime(),
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			solomonShard := SolomonShard{
				Project:       "alice-iot-sensors",
				ServicePrefix: "float-property",
				Cluster:       "beta",
			}
			actual := mapDevicePropertiesToMetrics(solomonShard, tc.deviceProps)
			for _, clusterMetrics := range actual {
				sort.SliceStable(clusterMetrics, func(i, j int) bool { // preserve order in test for correct comparison
					return clusterMetrics[i].Timestamp.Before(clusterMetrics[j].Timestamp)
				})
			}
			assert.Equal(t, tc.expected, actual)
		})
	}
}

func makeFloatProperty(lastUpdated timestamp.PastTimestamp, state model.FloatPropertyState) *model.FloatProperty {
	prop := &model.FloatProperty{}
	prop.SetLastUpdated(lastUpdated)
	prop.SetState(state)
	return prop
}

func makeEventProperty(lastUpdated timestamp.PastTimestamp, state model.EventPropertyState) *model.EventProperty {
	prop := &model.EventProperty{}
	prop.SetLastUpdated(lastUpdated)
	prop.SetState(state)
	return prop
}

func TestSolomonProgramRequest(t *testing.T) {
	shard := SolomonShard{
		Project:       "alice-iot-sensors",
		ServicePrefix: "float-property",
		Cluster:       "beta",
	}

	actual := solomonRequestForFloatProperty(shard, "52df797c-1049-419c-8b42-b2cb6086b82d", "humidity")
	expected := "{project=\"alice-iot-sensors\", cluster=\"beta\", service=\"float-property-humidity\", sensor=\"52df797c-1049-419c-8b42-b2cb6086b82d\"}"
	assert.Equal(t, expected, actual)
}
