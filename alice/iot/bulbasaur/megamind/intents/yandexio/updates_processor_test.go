package yandexio

import (
	"fmt"
	"reflect"
	"testing"

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"

	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	batterypb "a.yandex-team.ru/alice/protos/endpoint/capabilities/battery"
)

func Test_filterBatteryAndIlluminance(t *testing.T) {
	type args struct {
		capabilities []*anypb.Any
	}
	tests := []struct {
		name string
		args args
		want []*anypb.Any
	}{
		{
			"leave only illuminance and battery capabilities",
			args{
				capabilities: []*anypb.Any{
					mustNewAny(&endpointpb.TOnOffCapability{
						Meta: &endpointpb.TCapability_TMeta{
							SupportedEvents: []endpointpb.TCapability_EEventType{},
							SupportedDirectives: []endpointpb.TCapability_EDirectiveType{
								endpointpb.TCapability_OnOffDirectiveType,
							},
							Retrievable: true,
							Reportable:  true,
						},
						Parameters: &endpointpb.TOnOffCapability_TParameters{
							Split: false,
						},
						State: &endpointpb.TOnOffCapability_TState{
							On: true,
						},
					}),
					mustNewAny(&endpointpb.TLevelCapability{
						Meta: &endpointpb.TCapability_TMeta{
							SupportedEvents:     []endpointpb.TCapability_EEventType{},
							SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
							Retrievable:         false,
							Reportable:          true,
						},
						Parameters: &endpointpb.TLevelCapability_TParameters{
							Instance: endpointpb.TLevelCapability_TVOCInstance,
							Range: &endpointpb.TRange{
								Min:       0,
								Max:       100,
								Precision: 1,
							},
							Unit: endpointpb.EUnit_PPBUnit,
						},
						State: &endpointpb.TLevelCapability_TState{
							Level: 10000,
						},
					}),
					mustNewAny(&endpointpb.TLevelCapability{
						Meta: &endpointpb.TCapability_TMeta{
							SupportedEvents:     []endpointpb.TCapability_EEventType{},
							SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
							Retrievable:         false,
							Reportable:          true,
						},
						Parameters: &endpointpb.TLevelCapability_TParameters{
							Instance: endpointpb.TLevelCapability_IlluminanceInstance,
							Range: &endpointpb.TRange{
								Min:       0,
								Max:       100,
								Precision: 1,
							},
							Unit: endpointpb.EUnit_LuxUnit,
						},
						State: &endpointpb.TLevelCapability_TState{
							Level: 10,
						},
					}),
					mustNewAny(&batterypb.TBatteryCapability{
						Meta: &endpointpb.TCapability_TMeta{
							SupportedEvents:     []endpointpb.TCapability_EEventType{},
							SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
							Retrievable:         false,
							Reportable:          true,
						},
						Parameters: &batterypb.TBatteryCapability_TParameters{},
						State: &batterypb.TBatteryCapability_TState{
							Percentage: 99,
						},
					}),
				},
			},
			[]*anypb.Any{
				mustNewAny(&endpointpb.TLevelCapability{
					Meta: &endpointpb.TCapability_TMeta{
						SupportedEvents:     []endpointpb.TCapability_EEventType{},
						SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
						Retrievable:         false,
						Reportable:          true,
					},
					Parameters: &endpointpb.TLevelCapability_TParameters{
						Instance: endpointpb.TLevelCapability_IlluminanceInstance,
						Range: &endpointpb.TRange{
							Min:       0,
							Max:       100,
							Precision: 1,
						},
						Unit: endpointpb.EUnit_LuxUnit,
					},
					State: &endpointpb.TLevelCapability_TState{
						Level: 10,
					},
				}),
				mustNewAny(&batterypb.TBatteryCapability{
					Meta: &endpointpb.TCapability_TMeta{
						SupportedEvents:     []endpointpb.TCapability_EEventType{},
						SupportedDirectives: []endpointpb.TCapability_EDirectiveType{},
						Retrievable:         false,
						Reportable:          true,
					},
					Parameters: &batterypb.TBatteryCapability_TParameters{},
					State: &batterypb.TBatteryCapability_TState{
						Percentage: 99,
					},
				}),
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := filterBatteryAndIlluminance(tt.args.capabilities); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("filterBatteryAndIlluminance() = %v, want %v", got, tt.want)
			}
		})
	}
}

func mustNewAny(m proto.Message) *anypb.Any {
	value, err := anypb.New(m)
	if err != nil {
		panic(fmt.Sprintf("failed to create protobuf any value: %s", err))
	}
	return value
}
