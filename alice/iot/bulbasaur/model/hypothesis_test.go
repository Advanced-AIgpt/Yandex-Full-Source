package model_test

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/inflector"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestActionHypotheses(t *testing.T) {
	testCases := []struct {
		name             string
		createDevices    func() model.Devices
		createHouseholds func() model.Households
		scenarios        model.Scenarios
		createHypotheses func() model.Hypotheses
		isTandem         bool

		expectedExtractedActionsLength    int
		expectedExtractedActionsDeviceIDs [][]string
		expectedFiltrationResults         model.HypothesisFiltrationResults

		checkToDevices                         bool
		expectedDeviceIDsAfterDeviceConversion []string
	}{
		// TODO: remove after https://st.yandex-team.ru/IOT-1276 continues
		//{
		//	name: "search for onOff capability on tv from tandem, find irrelevant",
		//	createDevices: func() model.Devices {
		//		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		//		return model.Devices{
		//			*new(model.Device).WithID("device1").WithDeviceType(model.TvDeviceDeviceType).WithCapabilities(onOffCapability),
		//		}
		//	},
		//	scenarios: []model.Scenario{},
		//	createHypotheses: func() model.Hypotheses {
		//		hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
		//		hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
		//		return model.Hypotheses{*hypothesis.WithDevice("device1").WithHypothesisValue(hypothesisValue)}
		//	},
		//	isTandem: true,
		//
		//	expectedExtractedActionsLength:    0,
		//	expectedExtractedActionsDeviceIDs: [][]string{},
		//	expectedFiltrationResults: model.HypothesisFiltrationResults{
		//		{
		//			Reason:          model.TandemTVHypothesisFilterReason,
		//			SurvivedDevices: model.Devices{},
		//		},
		//	},
		//},
		{
			name: "search for onOff capability on lamp from tandem, find d1",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("device1").WithDeviceType(model.LightDeviceType).WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("device1").WithHypothesisValue(hypothesisValue)}
			},
			isTandem: true,

			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"device1"},
			},
		},
		{
			name: "search for onOff capability on tv from normal speaker, find d1",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("device1").WithDeviceType(model.TvDeviceDeviceType).WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("device1").WithHypothesisValue(hypothesisValue)}
			},
			isTandem: false,

			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"device1"},
			},
		},
		{
			name: "search for onOff capability, find d1",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("device1").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("device1").WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"device1"},
			},
		},
		{
			name: "search for OnOff capability, find d1 and filter device d2 with wrong capability type",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability),
					*new(model.Device).WithID("d2").WithCapabilities(colorCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0).WithDevices("d1", "d2")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1"},
			},
		},
		{
			name: "search for OnOff capability and group g1, find d1+d2 and filter device d3 with wrong group id",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)

				g1 := *model.NewGroup("g1").WithID("g1")
				g2 := *model.NewGroup("g2").WithID("g2")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithGroups(g1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithGroups(g1),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability).WithGroups(g2),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0).WithGroup("g1")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1", "d2"},
			},
		},
		{
			name: "search for OnOff capability and device d1, find d1 and deduplicate it after filtration",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue1 := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis1 := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0).WithDevice("d1")

				hypothesisValue2 := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis2 := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(1).WithDevice("d1")
				return model.Hypotheses{
					*hypothesis1.WithHypothesisValue(hypothesisValue1),
					*hypothesis2.WithHypothesisValue(hypothesisValue2),
				}
			},
			expectedExtractedActionsLength: 2,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1"},
				{"d1"},
			},
			checkToDevices: true,
			expectedDeviceIDsAfterDeviceConversion: []string{
				"d1",
			},
		},
		{
			name: "search for onOffCapability and rooms Kitchen+Bedroom, return inappropriate turn on all filter reason",
			// more details: https://st.yandex-team.ru/IOT-801
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)

				r1 := *model.NewRoom("Kitchen").WithID("Kitchen")
				r2 := *model.NewRoom("Bedroom").WithID("Bedroom")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithRoom(r2),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0).WithRoom("Kitchen").WithRoom("Bedroom")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 0,
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.InappropriateTurnOnAllDevicesFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "search for onOffCapability with no groups and no devices, return inappropriate turn on all filter reason",
			// more details: https://st.yandex-team.ru/IOT-801
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				r1 := *model.NewRoom("Kitchen").WithID("Kitchen")
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 0,
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.InappropriateTurnOnAllDevicesFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "search for onOffCapability with no groups and no devices, return inappropriate turn on all filter reason",
			// more details: https://st.yandex-team.ru/IOT-801
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				r1 := *model.NewRoom("Kitchen").WithID("Kitchen")
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 0,
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.InappropriateTurnOnAllDevicesFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "search for onOffCapability for all devices in different households",
			createHouseholds: func() model.Households {
				return model.Households{
					*model.NewHousehold("house 1").WithID("h1"),
					*model.NewHousehold("house 2").WithID("h2"),
				}
			},
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithHouseholdID("h1"),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithHouseholdID("h2"),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 0,
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.InappropriateTurnOnAllDevicesFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "search for onOffCapability for specific devices in the same households",
			createHouseholds: func() model.Households {
				return model.Households{
					*model.NewHousehold("house 1").WithID("h1"),
					*model.NewHousehold("house 2").WithID("h2"),
				}
			},
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithHouseholdID("h1"),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithHouseholdID("h2"),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability).WithHouseholdID("h1"),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).
					WithType(model.ActionHypothesisType).
					WithID(0).
					WithDevices("d1", "d3")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1", "d3"},
			},
			checkToDevices:                         true,
			expectedDeviceIDsAfterDeviceConversion: []string{"d1", "d3"},
		},
		{
			name: "search for onOffCapability for devices in different households",
			createHouseholds: func() model.Households {
				return model.Households{
					*model.NewHousehold("house 1").WithID("h1"),
					*model.NewHousehold("house 2").WithID("h2"),
				}
			},
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithHouseholdID("h1"),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithHouseholdID("h2"),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).
					WithType(model.ActionHypothesisType).
					WithID(0).
					WithDevices("d1", "d2")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 0,
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.ShouldSpecifyHouseholdFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "search for onOffCapability and group g1 and room Kitchen, find d1 and filter d2+d3",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)

				g1 := *model.NewGroup("g1").WithID("g1")
				r1 := *model.NewRoom("Kitchen").WithID("Kitchen")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithRoom(r1).WithGroups(g1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d3").WithCapabilities(onOffCapability).WithGroups(g1),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithID(0).WithRoom("Kitchen").WithGroup("g1")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1"},
			},
			checkToDevices:                         true,
			expectedDeviceIDsAfterDeviceConversion: []string{"d1"},
		},
		{
			name: "search for onOffCapability and group g1, find d1+d2, check that hypothesis without id is ok",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)

				g1 := *model.NewGroup("g1").WithID("g1")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithGroups(g1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithGroups(g1),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithGroup("g1")
				return model.Hypotheses{*hypothesis.WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedActionsLength: 1,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1", "d2"},
			},
			checkToDevices:                         true,
			expectedDeviceIDsAfterDeviceConversion: []string{"d1", "d2"},
		},
		{
			name: "2 hypothesis - {search for onOffCapability and group g1} and {search for OnOffCapability and device d1}, find d1+d2",
			createDevices: func() model.Devices {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)

				g1 := *model.NewGroup("g1").WithID("g1")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithGroups(g1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability),
				}
			},
			scenarios: []model.Scenario{},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue1 := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis1 := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithGroup("g1")

				hypothesisValue2 := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType).WithValue(true)
				hypothesis2 := new(model.Hypothesis).WithType(model.ActionHypothesisType).WithDevice("d2")
				return model.Hypotheses{
					*hypothesis1.WithHypothesisValue(hypothesisValue1),
					*hypothesis2.WithHypothesisValue(hypothesisValue2),
				}
			},
			expectedExtractedActionsLength: 2,
			expectedExtractedActionsDeviceIDs: [][]string{
				{"d1"}, {"d2"},
			},
			checkToDevices:                         true,
			expectedDeviceIDsAfterDeviceConversion: []string{"d1", "d2"},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			inflectorMock := inflector.NewInflectorMock(&nop.Logger{})
			userInfo := model.UserInfo{
				Devices:   tc.createDevices(),
				Scenarios: tc.scenarios,
			}
			if tc.createHouseholds != nil {
				userInfo.Households = tc.createHouseholds()
			}

			filtrationResults, extractedActions, _ := model.ExtractActions(
				inflectorMock,
				tc.createHypotheses(),
				userInfo,
				tc.isTandem,
				false,
			)
			require.Equal(t, tc.expectedExtractedActionsLength, len(extractedActions))
			if len(extractedActions) == 0 {
				require.ElementsMatch(t, tc.expectedFiltrationResults, filtrationResults)
			}
			for i := range extractedActions {
				t.Run(fmt.Sprintf("extractedActions[%d].DeviceIDs match", i), func(t *testing.T) {
					actualDeviceIDs := make([]string, 0)
					for _, device := range extractedActions[i].Devices {
						actualDeviceIDs = append(actualDeviceIDs, device.ID)
					}
					assert.ElementsMatch(t, tc.expectedExtractedActionsDeviceIDs[i], actualDeviceIDs)
				})
			}
			if tc.checkToDevices {
				t.Run("extractedActions.ToDevices().DeviceIDs match", func(t *testing.T) {
					actualDeviceIDs := make([]string, 0)
					for _, device := range extractedActions.ToDevices() {
						actualDeviceIDs = append(actualDeviceIDs, device.ID)
					}
					assert.ElementsMatch(t, tc.expectedDeviceIDsAfterDeviceConversion, actualDeviceIDs)
				})
			}
		})
	}
}

func TestQueryHypotheses(t *testing.T) {
	testCases := []struct {
		name             string
		createHouseholds func() model.Households
		createDevices    func() model.Devices
		createHypotheses func() model.Hypotheses
		isTandem         bool

		expectedExtractedQueriesLength    int
		expectedExtractedQueriesDeviceIDs [][]string
		expectedFiltrationResults         model.HypothesisFiltrationResults

		checkAttrs    bool
		expectedAttrs []model.ExtractedQueryAttributes
	}{
		{
			name: "query {tv d1 on?} from tandem, expect irrelevant",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)
				return model.Devices{
					*new(model.Device).WithID("d1").WithDeviceType(model.TvDeviceDeviceType).WithCapabilities(onOffCapability),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("d1").WithHypothesisValue(hypothesisValue)}
			},
			isTandem: true,

			expectedExtractedQueriesLength:    0,
			expectedExtractedQueriesDeviceIDs: [][]string{},
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.TandemTVHypothesisFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
		{
			name: "query {lamp d1 on?} from tandem, expect d1",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)
				return model.Devices{
					*new(model.Device).WithID("d1").WithDeviceType(model.LightDeviceType).WithCapabilities(onOffCapability),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("d1").WithHypothesisValue(hypothesisValue)}
			},
			isTandem: true,

			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1"},
			},

			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Devices: []string{"d1"},
				},
			},
		},
		{
			name: "query {tv d1 on?} from normal speaker, expect d1",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)
				return model.Devices{
					*new(model.Device).WithID("d1").WithDeviceType(model.TvDeviceDeviceType).WithCapabilities(onOffCapability),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("d1").WithHypothesisValue(hypothesisValue)}
			},
			isTandem: false,

			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1"},
			},

			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Devices: []string{"d1"},
				},
			},
		},
		{
			name: "query {device d1 on?}, expect d1",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)
				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevice("d1").WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1"},
			},
			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Devices: []string{"d1"},
				},
			},
		},
		{
			name: "query {group g1 on?}, expect d1+d2 and g1 in attrs",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)

				g1 := *model.NewGroup("group name 1").WithID("g1")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithGroups(g1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithGroups(g1),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{
					*hypothesis.WithGroup("g1").WithHypothesisValue(hypothesisValue),
				}
			},
			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1", "d2"},
			},
			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Groups: []string{"g1"},
				},
			},
		},
		{
			name: "query {room r1 on?}, expect d1+d2 and r1 in attrs",
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)

				r1 := *model.NewRoom("room name 1").WithID("r1")

				return model.Devices{
					*new(model.Device).WithID("d1").WithCapabilities(onOffCapability).WithRoom(r1),
					*new(model.Device).WithID("d2").WithCapabilities(onOffCapability).WithRoom(r1),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{
					*hypothesis.WithRoom("r1").WithHypothesisValue(hypothesisValue),
				}
			},
			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1", "d2"},
			},
			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Rooms: []string{"r1"},
				},
			},
		},
		{
			name: "query {group g1, room r1, battery level?}, expect d1 and g1+r1 in attrs",
			createDevices: func() model.Devices {
				batteryLevel := model.MakePropertyByType(model.FloatPropertyType)
				batteryLevel.SetRetrievable(true)
				batteryLevel.SetParameters(model.FloatPropertyParameters{
					Instance: model.BatteryLevelPropertyInstance,
					Unit:     model.UnitPercent,
				})
				r1 := *model.NewRoom("room name 1").WithID("r1")
				g1 := *model.NewGroup("group name 1").WithID("g1")
				return model.Devices{
					*new(model.Device).WithID("d1").WithProperties(batteryLevel).WithGroups(g1).WithRoom(r1),
					*new(model.Device).WithID("d2").WithProperties(batteryLevel).WithGroups(g1),
					*new(model.Device).WithID("d3").WithProperties(batteryLevel).WithRoom(r1),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithPropertyType(model.FloatPropertyType).WithInstance(string(model.BatteryLevelPropertyInstance))
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{
					*hypothesis.WithGroup("g1").WithRoom("r1").WithHypothesisValue(hypothesisValue),
				}
			},
			expectedExtractedQueriesLength: 1,
			expectedExtractedQueriesDeviceIDs: [][]string{
				{"d1"},
			},
			checkAttrs: true,
			expectedAttrs: []model.ExtractedQueryAttributes{
				{
					Rooms:  []string{"r1"},
					Groups: []string{"g1"},
				},
			},
		},
		{
			name: "query {tv d1 on?} devices from different households",
			createHouseholds: func() model.Households {
				return model.Households{
					*model.NewHousehold("house 1").WithID("h1"),
					*model.NewHousehold("house 2").WithID("h2"),
				}
			},
			createDevices: func() model.Devices {
				onOffCapability := model.NewCapability(model.OnOffCapabilityType).WithRetrievable(true)
				return model.Devices{
					*new(model.Device).WithID("d1").
						WithDeviceType(model.TvDeviceDeviceType).
						WithCapabilities(onOffCapability).
						WithHouseholdID("h1"),
					*new(model.Device).WithID("d2").
						WithDeviceType(model.TvDeviceDeviceType).
						WithCapabilities(onOffCapability).
						WithHouseholdID("h2"),
				}
			},
			createHypotheses: func() model.Hypotheses {
				hypothesisValue := *model.NewActionHypothesis().WithCapabilityType(model.OnOffCapabilityType)
				hypothesis := new(model.Hypothesis).WithType(model.QueryHypothesisType).WithID(0)
				return model.Hypotheses{*hypothesis.WithDevices("d1", "d2").WithHypothesisValue(hypothesisValue)}
			},
			expectedExtractedQueriesLength:    0,
			expectedExtractedQueriesDeviceIDs: [][]string{},
			expectedFiltrationResults: model.HypothesisFiltrationResults{{
				Reason:          model.ShouldSpecifyHouseholdFilterReason,
				SurvivedDevices: model.Devices{},
			}},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			userInfo := model.UserInfo{
				Devices: tc.createDevices(),
			}

			if tc.createHouseholds != nil {
				userInfo.Households = tc.createHouseholds()
			}

			filtrationResults, extractedQueries := model.ExtractQueries(tc.createHypotheses(), userInfo, tc.isTandem, false)
			require.Equal(t, tc.expectedExtractedQueriesLength, len(extractedQueries))
			if tc.expectedExtractedQueriesLength == 0 {
				require.ElementsMatch(t, tc.expectedFiltrationResults, filtrationResults)
			}
			for i := range extractedQueries {
				t.Run(fmt.Sprintf("extractedQueries[%d].DeviceIDs match", i), func(t *testing.T) {
					actualDeviceIDs := make([]string, 0)
					for _, device := range extractedQueries[i].Devices {
						actualDeviceIDs = append(actualDeviceIDs, device.ID)
					}
					assert.ElementsMatch(t, tc.expectedExtractedQueriesDeviceIDs[i], actualDeviceIDs)
				})
			}
		})
	}
}

// these 6 tests test IsActionApplicable

func TestIsOnOffActionApplicable(t *testing.T) {
	c := model.NewCapability(model.OnOffCapabilityType)
	d := model.NewDevice("big iron").WithDeviceType(model.IronDeviceType)
	relativity := model.Invert
	hv := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     c.Type().String(),
		Instance: c.Instance(),
		Relative: &relativity,
	}

	var actualValue bool
	assert.NotPanics(t, func() {
		actualValue = model.IsActionApplicable(*d, c, hv)
	})
	assert.False(t, actualValue)
}

func TestFilterRangeCapabilityAction(t *testing.T) {
	c := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.VolumeRangeInstance,
			RandomAccess: false,
			Range: &model.Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
		})
	d := model.NewDevice("d").WithID("d").WithCapabilities(*c)
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     c.Type().String(),
		Instance: c.Instance(),
		Value:    50.0,
	}

	filtered := model.FilterHypothesisValue([]model.Device{*d}, model.ActionHypothesisType, a)
	assert.Equal(t, 0, len(filtered.SurvivedDevices))
}

func TestColorCapabilityAction(t *testing.T) {
	onlyWhite := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			TemperatureK: &model.TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		})
	bothColorModes := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.RgbModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		})
	onlyScenes := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID:   model.ColorSceneIDSiren,
						Name: model.KnownColorScenes[model.ColorSceneIDSiren].Name,
					},
					{
						ID:   model.ColorSceneIDParty,
						Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
					},
				},
			},
		})
	onlyWhiteDevice := model.NewDevice("onlyWhite").WithID("d").WithCapabilities(*onlyWhite)
	bothModesDevice := model.NewDevice("bothModes").WithID("d").WithCapabilities(*bothColorModes)
	onlyScenesDevice := model.NewDevice("onlyScenes").WithID("onlyScenes").WithCapabilities(*onlyScenes)
	t.Run("warm_white color is applicable on white device", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyWhite.Type().String(),
			Instance: model.HypothesisColorCapabilityInstance,
			Value:    "warm_white",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*onlyWhiteDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 1, len(filtered.SurvivedDevices))
	})
	t.Run("red color is NOT applicable on white device", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyWhite.Type().String(),
			Instance: model.HypothesisColorCapabilityInstance,
			Value:    "red",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*onlyWhiteDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 0, len(filtered.SurvivedDevices))
	})
	t.Run("warm_white color is applicable on both modes", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     model.ColorSettingCapabilityType.String(),
			Instance: model.HypothesisColorCapabilityInstance,
			Value:    "warm_white",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*bothModesDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 1, len(filtered.SurvivedDevices))
	})
	t.Run("red color is applicable on both device", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyWhite.Type().String(),
			Instance: model.HypothesisColorCapabilityInstance,
			Value:    "red",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*bothModesDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 1, len(filtered.SurvivedDevices))
	})
	t.Run("scene is applicable on only scenes", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyScenes.Type().String(),
			Instance: "color_scene",
			Value:    "siren",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*onlyScenesDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 1, len(filtered.SurvivedDevices))
	})
	t.Run("scene is NOT applicable on only whites", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyWhite.Type().String(),
			Instance: "color_scene",
			Value:    "siren",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*onlyWhiteDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 0, len(filtered.SurvivedDevices))
	})
	t.Run("scene is NOT applicable on both modes", func(t *testing.T) {
		a := model.HypothesisValue{
			Target:   model.CapabilityTarget,
			Type:     onlyWhite.Type().String(),
			Instance: "color_scene",
			Value:    "siren",
		}

		filtered := model.FilterHypothesisValue([]model.Device{*bothModesDevice}, model.ActionHypothesisType, a)
		assert.Equal(t, 0, len(filtered.SurvivedDevices))
	})
}

func TestNilRanges(t *testing.T) {
	// Case 1: try to set max volume on range with nil range params
	rangeVolumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		})

	device := model.NewDevice("d1").WithCapabilities(rangeVolumeCapability)

	h := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h.WithDevice(device.ID)
	h.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithValue("max"))

	userInfo := model.UserInfo{
		Devices:   []model.Device{*device},
		Scenarios: []model.Scenario{},
	}
	_, filtered, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 0, len(filtered))

	// Case 2: valid ir tv channel range with random access
	rangeChannelCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			RandomAccess: true,
			Looped:       false,
		})

	device2 := model.NewDevice("d2").WithCapabilities(rangeChannelCapability)

	h2 := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h2.WithDevice(device2.ID)
	h2.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.ChannelRangeInstance)).
		WithValue(float64(5)))

	userInfo = model.UserInfo{
		Devices:   []model.Device{*device2},
		Scenarios: []model.Scenario{},
	}
	_, filtered2, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h2},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 1, len(filtered2))

	// Case 3: try to increase volume on range with nil range params
	rangeVolumeCapability2 := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		})

	device3 := model.NewDevice("d3").WithCapabilities(rangeVolumeCapability2)

	h3 := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h3.WithDevice(device3.ID)
	h3.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase))

	userInfo = model.UserInfo{
		Devices:   []model.Device{*device3},
		Scenarios: []model.Scenario{},
	}
	_, filtered3, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h3},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 1, len(filtered3))

	// Case 3.1: try to increase volume on range with nil range params and retrievable true
	// Case from https://st.yandex-team.ru/QUASARSUP-1732
	rangeVolumeCapability31 := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		}).
		WithRetrievable(true)

	device31 := model.NewDevice("d31").WithCapabilities(rangeVolumeCapability31)

	h31 := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h31.WithDevice(device31.ID)
	h31.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase))

	userInfo = model.UserInfo{
		Devices:   []model.Device{*device31},
		Scenarios: []model.Scenario{},
	}
	_, filtered31, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h31},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 1, len(filtered31))

	// Case 4: try to increase volume at 10% on range with nil range params
	rangeVolumeCapability3 := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		})

	device4 := model.NewDevice("d4").WithCapabilities(rangeVolumeCapability3)

	h4 := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h4.WithDevice(device4.ID)
	h4.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithUnit(model.UnitPercent).
		WithValue(float64(10)))

	userInfo = model.UserInfo{
		Devices:   []model.Device{*device4},
		Scenarios: []model.Scenario{},
	}
	_, filtered4, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h4},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 0, len(filtered4))
}

func TestHypothesesTvWithVolumeIncrease(t *testing.T) {
	rangeVolumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		}).
		WithRetrievable(false)

	device := model.NewDevice("d").
		WithID("d").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.TvDeviceDeviceType)
	device2 := model.NewDevice("d2").
		WithID("d2").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.TvDeviceDeviceType)
	device3 := model.NewDevice("d3").
		WithID("d3").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.TvDeviceDeviceType)
	device4 := model.NewDevice("d4").
		WithID("d4").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.TvDeviceDeviceType)
	device5 := model.NewDevice("d5").
		WithID("d5").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID("Some-skill-id").
		WithDeviceType(model.TvDeviceDeviceType)
	device6 := model.NewDevice("d6").
		WithID("d6").
		WithCapabilities(*rangeVolumeCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.AcDeviceType)

	h := model.NewHypothesis().WithID(0).WithType(model.ActionHypothesisType)
	h.WithDevice(device.ID)
	h.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(25)))
	h2 := model.NewHypothesis().WithID(2).WithType(model.ActionHypothesisType)
	h2.WithDevice(device2.ID)
	h2.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(50)))
	h3 := model.NewHypothesis().WithID(3).WithType(model.ActionHypothesisType)
	h3.WithDevice(device3.ID)
	h3.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(51)))
	h4 := model.NewHypothesis().WithID(4).WithType(model.ActionHypothesisType)
	h4.WithDevice(device4.ID)
	h4.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(75)))
	h5 := model.NewHypothesis().WithID(5).WithType(model.ActionHypothesisType)
	h5.WithDevice(device5.ID)
	h5.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(75)))
	h6 := model.NewHypothesis().WithID(6).WithType(model.ActionHypothesisType)
	h6.WithDevice(device6.ID)
	h6.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.VolumeRangeInstance)).
		WithRelative(model.Increase).
		WithValue(float64(75)))

	// Non-relative channel setting
	// case from https://st.yandex-team.ru/QUASARSUP-1857
	channelCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			RandomAccess: true,
		}).
		WithRetrievable(false)
	device7 := model.NewDevice("d7").
		WithCapabilities(*channelCapability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.TvDeviceDeviceType)
	h7 := model.NewHypothesis().WithID(7).WithType(model.ActionHypothesisType)
	h7.WithDevice(device7.ID)
	h7.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.ChannelRangeInstance)).
		WithValue(float64(51)))

	userInfo := model.UserInfo{
		Devices:   []model.Device{*device, *device2, *device3, *device4, *device5, *device6, *device7},
		Scenarios: []model.Scenario{},
	}
	_, filteredDevices, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h, *h2, *h3, *h4, *h5, *h6, *h7},
		userInfo,
		false,
		false,
	)

	assert.Len(t, filteredDevices, 4)
}

func TestHypothesesAcWithoutStateRelativeIncrease(t *testing.T) {
	capability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.TemperatureRangeInstance,
			Range: &model.Range{
				Min:       18,
				Max:       28,
				Precision: 1,
			},
		}).
		WithRetrievable(true)

	device := model.NewDevice("d").
		WithCapabilities(*capability).
		WithSkillID(model.TUYA).
		WithDeviceType(model.AcDeviceType)

	h := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h.WithDevice(device.ID)
	h.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.RangeCapabilityType).
		WithInstance(string(model.TemperatureRangeInstance)).
		WithRelative(model.Increase).
		WithUnit(model.UnitTemperatureCelsius).
		WithValue(float64(1)))

	userInfo := model.UserInfo{
		Devices:   []model.Device{*device},
		Scenarios: []model.Scenario{},
	}
	_, filtered, _ := model.ExtractActions(
		nil,
		[]model.Hypothesis{*h},
		userInfo,
		false,
		false,
	)

	assert.Equal(t, 1, len(filtered))
}

func makeSimpleTimeInfo(t time.Time, isTimeSpecified bool) model.TimeInfo {
	return model.TimeInfo{
		DateTime: model.DateTime{
			Time:            t,
			IsTimeSpecified: isTimeSpecified,
		},
	}
}

func TestDatetimeValidation(t *testing.T) {
	testNow := time.Date(2020, 10, 28, 20, 46, 0, 0, time.UTC)

	inputs := []struct {
		Name            string
		Now             time.Time
		TimeInfo        model.TimeInfo
		ExpectedIsValid bool
		ExpectedNLG     libnlg.NLG
	}{
		{
			Name:            "Включи розетку через 5 минут",
			Now:             testNow,
			TimeInfo:        makeSimpleTimeInfo(testNow.Add(5*time.Minute), true),
			ExpectedIsValid: true,
			ExpectedNLG:     nil,
		},
		{
			Name:            "Включи розетку через год - сразу отдаем, что ошибка, не пытаемся уточнить время",
			Now:             testNow,
			TimeInfo:        makeSimpleTimeInfo(testNow.AddDate(1, 0, 0), false),
			ExpectedIsValid: false,
			ExpectedNLG:     nlg.FutureAction,
		},
		{
			Name:            "Включи розетку сегодня - считаем, что валидно, т.к. может быть как в будущем, так и в прошлом",
			Now:             testNow,
			TimeInfo:        makeSimpleTimeInfo(testNow, false),
			ExpectedIsValid: true,
			ExpectedNLG:     nil,
		},
		{
			Name:            "Включи розетку вчера - сразу отдаем, что ошибка, не пытаемся уточнить время",
			Now:             testNow,
			TimeInfo:        makeSimpleTimeInfo(testNow.Add(-24*time.Hour), false),
			ExpectedIsValid: false,
			ExpectedNLG:     nlg.PastAction,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			response, err := input.TimeInfo.Validate(input.Now)
			if input.ExpectedIsValid {
				assert.NoError(t, err)
				return
			}

			assert.Error(t, err)
			assert.Equal(t, input.ExpectedNLG, response)
		})
	}
}

func TestHypothesisFiltrationResultMerge(t *testing.T) {
	type testCase struct {
		Name           string
		Results        model.HypothesisFiltrationResults
		ExpectedResult model.HypothesisFiltrationResult
	}
	testDevices := []model.Device{{ID: "1"}, {ID: "2"}, {ID: "3"}}
	testCases := []testCase{
		{
			Name: "got bad in first case",
			Results: model.HypothesisFiltrationResults{
				{
					Reason:          model.InappropriateRoomFilterReason,
					SurvivedDevices: []model.Device{},
				},
				{
					Reason:          model.AllGoodFilterReason,
					SurvivedDevices: []model.Device{},
				},
			},
			ExpectedResult: model.HypothesisFiltrationResult{
				Reason:          model.InappropriateRoomFilterReason,
				SurvivedDevices: []model.Device{},
			},
		},
		{
			Name: "all good",
			Results: model.HypothesisFiltrationResults{
				{
					Reason:          model.AllGoodFilterReason,
					SurvivedDevices: testDevices,
				},
				{
					Reason:          model.AllGoodFilterReason,
					SurvivedDevices: testDevices[:1],
				},
			},
			ExpectedResult: model.HypothesisFiltrationResult{
				Reason:          model.AllGoodFilterReason,
				SurvivedDevices: testDevices[:1],
			},
		},
	}
	for _, testCase := range testCases {
		t.Run(testCase.Name, func(t *testing.T) {
			result := model.NewHypothesisFiltrationResult(testDevices)
			for _, other := range testCase.Results {
				result.Merge(other)
			}

			assert.Equal(t, testCase.ExpectedResult, result)
		})
	}
}

func TestHypothesisExtractActionsResult(t *testing.T) {
	hypotheses := []model.Hypothesis{
		{
			ID:      1,
			Devices: []string{"1", "2", "3"},
			Rooms:   []string{"1"},
			Groups:  []string{},
			Type:    model.ActionHypothesisType,
			Value: model.HypothesisValue{
				Target:   model.CapabilityTarget,
				Type:     string(model.OnOffCapabilityType),
				Instance: string(model.OnOnOffCapabilityInstance),
				Value:    true,
			},
		},
	}
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	devices := model.Devices{
		{
			ID:           "1",
			Capabilities: model.Capabilities{onOff},
			Room: &model.Room{
				ID:   "2",
				Name: "Зал",
			},
		},
		{
			ID:           "2",
			Capabilities: model.Capabilities{onOff},
			Room: &model.Room{
				ID:   "2",
				Name: "Зал",
			},
		},
		{
			ID:           "4",
			Capabilities: model.Capabilities{onOff},
			Room: &model.Room{
				ID:   "1",
				Name: "Ванная",
			},
		},
		{
			ID:           "5",
			Capabilities: model.Capabilities{onOff},
			Room: &model.Room{
				ID:   "2",
				Name: "Ванная",
			},
		},
	}

	userInfo := model.UserInfo{
		Devices:   devices,
		Scenarios: model.Scenarios{},
	}
	results, _, _ := model.ExtractActions(
		nil,
		hypotheses,
		userInfo,
		false,
		false,
	)
	expectedRes := model.HypothesisFiltrationResult{
		Reason:          model.InappropriateRoomFilterReason,
		SurvivedDevices: model.Devices{},
	}
	assert.Len(t, results, 1)
	assert.Equal(t, expectedRes, results[0])
}

func TestHypothesisExtractActionsHouseholdResult(t *testing.T) {
	hypotheses := []model.Hypothesis{
		{
			ID:         1,
			Devices:    []string{"1"},
			Rooms:      []string{"2"},
			Groups:     []string{},
			Households: []string{"2"},
			Type:       model.ActionHypothesisType,
			Value: model.HypothesisValue{
				Target:   model.CapabilityTarget,
				Type:     string(model.OnOffCapabilityType),
				Instance: string(model.OnOnOffCapabilityInstance),
				Value:    true,
			},
		},
	}
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	devices := model.Devices{
		{
			ID:           "1",
			Capabilities: model.Capabilities{onOff},
			HouseholdID:  "1",
			Room: &model.Room{
				ID:   "2",
				Name: "Зал",
			},
		},
		{
			ID:           "2",
			Capabilities: model.Capabilities{onOff},
			HouseholdID:  "2",
			Room: &model.Room{
				ID:   "2",
				Name: "Зал",
			},
		},
		{
			ID:           "4",
			Capabilities: model.Capabilities{onOff},
			HouseholdID:  "1",
			Room: &model.Room{
				ID:   "1",
				Name: "Ванная",
			},
		},
		{
			ID:           "5",
			Capabilities: model.Capabilities{onOff},
			HouseholdID:  "1",
			Room: &model.Room{
				ID:   "2",
				Name: "Ванная",
			},
		},
	}

	userInfo := model.UserInfo{
		Devices:   devices,
		Scenarios: []model.Scenario{},
	}
	results, _, _ := model.ExtractActions(
		nil,
		hypotheses,
		userInfo,
		false,
		false,
	)
	expectedRes := model.HypothesisFiltrationResult{
		Reason:          model.InappropriateHouseholdFilterReason,
		SurvivedDevices: model.Devices{},
	}
	assert.Len(t, results, 1)
	assert.Equal(t, expectedRes, results[0])
}

func TestHypothesesPopulateWithHouseholdSpecifiedNLG(t *testing.T) {
	hypotheses := model.Hypotheses{
		{
			NLG: model.NLGStruct{Variants: []string{"ща как включу", "уу"}},
		},
		{
			NLG: model.NLGStruct{Variants: []string{"себе что-нибудь включи", "уу"}},
		},
	}
	householdInflection := inflector.Inflection{
		Im:   "дача",
		Rod:  "дачи",
		Dat:  "даче",
		Vin:  "дачу",
		Tvor: "дачей",
		Pr:   "даче",
	}
	expected := model.Hypotheses{
		{
			NLG: model.NLGStruct{Variants: []string{
				"Окей, сделала на даче",
				"Сделала на даче",
				"Выполнила на даче",
			}},
		},
		{
			NLG: model.NLGStruct{Variants: []string{
				"Окей, сделала на даче",
				"Сделала на даче",
				"Выполнила на даче",
			}},
		},
	}
	hypotheses.PopulateWithHouseholdSpecifiedNLG(householdInflection)
	assert.Equal(t, expected, hypotheses)
}

func TestHypothesesMergeDevicesHouseholds(t *testing.T) {
	hypothesis := model.Hypothesis{Devices: []string{"1", "2", "3"}}
	userInfo := model.UserInfo{
		Devices: model.Devices{
			{
				ID:          "1",
				HouseholdID: "household-1",
			},
			{
				ID:          "2",
				HouseholdID: "household-1",
			},
			{
				ID:          "3",
				HouseholdID: "household-1",
			},
		},
		Groups: model.Groups{
			{
				ID:          "group-1",
				HouseholdID: "household-2",
			},
		},
		Households: model.Households{
			{
				ID:   "household-1",
				Name: "Мой дом",
			},
			{
				ID:   "household-2",
				Name: "Дача",
			},
		},
	}
	expected := model.Household{
		ID:   "household-1",
		Name: "Мой дом",
	}
	mergedHousehold, successfulMerge := hypothesis.MergeHouseholds(userInfo)
	assert.True(t, successfulMerge)
	assert.Equal(t, expected, mergedHousehold)

	hypothesis = model.Hypothesis{Groups: []string{"group-1"}}
	expected = model.Household{
		ID:   "household-2",
		Name: "Дача",
	}
	mergedHousehold, successfulMerge = hypothesis.MergeHouseholds(userInfo)
	assert.True(t, successfulMerge)
	assert.Equal(t, expected, mergedHousehold)
}
