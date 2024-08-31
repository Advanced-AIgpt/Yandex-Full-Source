package processors

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/ptr"
)

func TestActionProcessor(t *testing.T) {
	suite.Run(t, new(ActionProcessorTestSuite))
}

type ActionProcessorTestSuite struct {
	suite.Suite

	actionProcessor ActionProcessor
	ctx             context.Context
}

func (suite *ActionProcessorTestSuite) SetupSuite() {
	suite.actionProcessor = ActionProcessor{
		Logger:             &nop.Logger{},
		inflector:          nil,
		scenarioController: nil,
		actionController:   nil,
	}

	timestamper := timestamp.TimestamperMock{
		CreatedTimestampValue: timestamp.FromTime(time.Date(2020, 1, 1, 0, 0, 0, 0, time.UTC)),
		CurrentTimestampValue: timestamp.FromTime(time.Date(2020, 1, 1, 0, 1, 0, 0, time.UTC)),
	}
	suite.ctx = timestamp.ContextWithTimestamper(context.Background(), &timestamper)
}

func (suite *ActionProcessorTestSuite) TestCheckBegemotDateTime() {
	processorContext := common.RunProcessorContext{
		Context:  suite.ctx,
		UserInfo: model.UserInfo{},
	}

	inputs := []struct {
		Name        string
		BegemotDate common.BegemotDate
		BegemotTime common.BegemotTime
		ExpectedOk  bool
		ExpectedNLG libnlg.NLG
	}{
		{
			Name:        "no_datetime_specified",
			BegemotDate: common.BegemotDate{},
			BegemotTime: common.BegemotTime{},
			ExpectedOk:  true,
		},
		{
			Name: "datetime_filled_with_zeros",
			BegemotDate: common.BegemotDate{
				Years:          ptr.Int(0),
				Months:         ptr.Int(0),
				Weeks:          ptr.Int(0),
				Days:           ptr.Int(0),
				Weekday:        ptr.Int(0),
				YearsRelative:  false,
				MonthsRelative: false,
				WeeksRelative:  false,
				DaysRelative:   false,
				DateRelative:   false,
			},
			BegemotTime: common.BegemotTime{
				Hours:           ptr.Int(0),
				Minutes:         ptr.Int(0),
				Seconds:         ptr.Int(0),
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
			ExpectedOk: true,
		},
		{
			Name: "only_date_specified",
			BegemotDate: common.BegemotDate{
				Days:         ptr.Int(2),
				DaysRelative: true,
			},
			BegemotTime: common.BegemotTime{},
			ExpectedOk:  false,
			ExpectedNLG: nlg.NoTimeSpecified,
		},
		{
			Name:        "only_time_specified",
			BegemotDate: common.BegemotDate{},
			BegemotTime: common.BegemotTime{
				Hours: ptr.Int(18),
			},
			ExpectedOk: true,
		},
		{
			Name: "date_is_relative_time_is_not",
			BegemotDate: common.BegemotDate{
				Days:         ptr.Int(2),
				DaysRelative: true,
			},
			BegemotTime: common.BegemotTime{
				Hours: ptr.Int(18),
			},
			ExpectedOk: true,
		},
		{
			Name: "time_is_relative_date_is_not",
			BegemotDate: common.BegemotDate{
				Days:   ptr.Int(2),
				Months: ptr.Int(5),
			},
			BegemotTime: common.BegemotTime{
				Minutes:         ptr.Int(20),
				MinutesRelative: true,
			},
			ExpectedOk:  false,
			ExpectedNLG: nlg.WeirdDateTimeRelativity,
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			actionFrame := frames.ActionFrameBuilder{}
			actionFrame.WithDate(input.BegemotDate).WithTime(input.BegemotTime)

			var checkResult checkStatus
			suite.NotPanics(func() {
				checkResult = suite.actionProcessor.checkBegemotDateTime(processorContext, actionFrame.Build())
			})

			suite.Equal(input.ExpectedOk, checkResult.ok)

			if input.ExpectedOk {
				return
			}

			suite.True(runResponseHasCorrectNLG(checkResult.runResponse, input.ExpectedNLG))
		})
	}
}

func (suite *ActionProcessorTestSuite) TestCheckParsedTime() {
	processorContext := common.RunProcessorContext{
		Context:  suite.ctx,
		UserInfo: model.UserInfo{},
	}

	inputs := []struct {
		Name        string
		ParsedTime  time.Time
		ClientTime  time.Time
		ExpectedOk  bool
		ExpectedNLG libnlg.NLG
	}{
		{
			Name:       "parsed_time_is_zero",
			ParsedTime: time.Time{},
			ClientTime: time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ExpectedOk: true,
		},
		{
			Name:       "parsed_time_is_now",
			ParsedTime: time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ClientTime: time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ExpectedOk: true,
		},
		{
			Name:        "parsed_time_is_in_the_past_1",
			ParsedTime:  time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ClientTime:  time.Date(2000, 1, 1, 0, 0, 0, 2, time.UTC),
			ExpectedOk:  false,
			ExpectedNLG: nlg.PastAction,
		},
		{
			Name:        "parsed_time_is_in_the_past_2",
			ParsedTime:  time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ClientTime:  time.Date(2001, 1, 1, 0, 0, 0, 1, time.UTC),
			ExpectedOk:  false,
			ExpectedNLG: nlg.PastAction,
		},
		{
			Name:       "parsed_time_is_in_the_near_future",
			ParsedTime: time.Date(2000, 1, 4, 13, 37, 3, 22, time.UTC),
			ClientTime: time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ExpectedOk: true,
		},
		{
			Name:        "parsed_time_is_in_the_far_future",
			ParsedTime:  time.Date(2000, 1, 9, 13, 37, 3, 22, time.UTC),
			ClientTime:  time.Date(2000, 1, 1, 0, 0, 0, 1, time.UTC),
			ExpectedOk:  false,
			ExpectedNLG: nlg.FutureAction,
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			actionFrame := frames.ActionFrameBuilder{}
			actionFrame.WithParsedTime(input.ParsedTime)

			var checkResult checkStatus
			suite.NotPanics(func() {
				checkResult = suite.actionProcessor.checkParsedTime(processorContext, actionFrame.Build(), input.ClientTime)
			})

			suite.Equal(input.ExpectedOk, checkResult.ok)
			if input.ExpectedOk {
				return
			}

			suite.True(runResponseHasCorrectNLG(checkResult.runResponse, input.ExpectedNLG))
		})
	}
}

func (suite *ActionProcessorTestSuite) TestCheckHouseholds() {
	processorContext := common.RunProcessorContext{
		Context:  suite.ctx,
		UserInfo: model.UserInfo{},
	}

	inputs := []struct {
		Name              string
		UserDevices       model.Devices
		FrameDeviceIDs    []string
		FrameHouseholdIDs []string
		ExpectedOk        bool
		ExpectedNLG       libnlg.NLG
	}{
		{
			Name:        "empty_frame_empty_user",
			UserDevices: model.Devices{},
			ExpectedOk:  true,
		},
		{
			Name:              "empty_user",
			UserDevices:       model.Devices{},
			FrameDeviceIDs:    []string{"device-1"},
			FrameHouseholdIDs: []string{"household-1"},
			ExpectedOk:        true,
		},
		{
			Name: "empty_frame_devices",
			UserDevices: model.Devices{
				model.Device{
					ID:          "device-1",
					HouseholdID: "household-1",
				},
			},
			FrameHouseholdIDs: []string{"household-1"},
			ExpectedOk:        true,
		},
		{
			Name: "no_household_id_all_devices_in_the_same_household",
			UserDevices: model.Devices{
				model.Device{
					ID:          "device-1",
					HouseholdID: "household-1",
				},
				model.Device{
					ID:          "device-2",
					HouseholdID: "household-1",
				},
			},
			FrameDeviceIDs: []string{"device-1", "device-2"},
			ExpectedOk:     true,
		},
		{
			Name: "no_household_id_devices_in_different_households",
			UserDevices: model.Devices{
				model.Device{
					ID:          "device-1",
					HouseholdID: "household-1",
				},
				model.Device{
					ID:          "device-2",
					HouseholdID: "household-2",
				},
			},
			FrameDeviceIDs: []string{"device-1", "device-2"},
			ExpectedOk:     false,
			ExpectedNLG:    nlg.NoHouseholdSpecifiedAction,
		},
		{
			Name: "multiple_households",
			UserDevices: model.Devices{
				model.Device{
					ID:          "device-1",
					HouseholdID: "household-1",
				},
				model.Device{
					ID:          "device-2",
					HouseholdID: "household-2",
				},
			},
			FrameDeviceIDs:    []string{"device-1", "device-2"},
			FrameHouseholdIDs: []string{"household-1", "household-2"},
			ExpectedOk:        false,
			ExpectedNLG:       nlg.MultipleHouseholdsInRequest,
		},
		{
			Name:              "multiple_households_no_devices",
			UserDevices:       model.Devices{},
			FrameHouseholdIDs: []string{"household-1", "household-2"},
			ExpectedOk:        false,
			ExpectedNLG:       nlg.MultipleHouseholdsInRequest,
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			actionFrame := frames.ActionFrameBuilder{}
			actionFrame.WithDeviceIDs(input.FrameDeviceIDs...).WithHouseholdIDs(input.FrameHouseholdIDs...)
			processorContext.UserInfo = model.UserInfo{
				Devices: input.UserDevices,
			}

			var checkResult checkStatus
			suite.NotPanics(func() {
				checkResult = suite.actionProcessor.checkHouseholds(processorContext, actionFrame.Build())
			})

			suite.Equal(input.ExpectedOk, checkResult.ok)
			if input.ExpectedOk {
				return
			}

			suite.True(runResponseHasCorrectNLG(checkResult.runResponse, input.ExpectedNLG))
		})
	}
}

func (suite *ActionProcessorTestSuite) TestValidate() {
	inputs := []struct {
		Name         string
		DeviceIDs    []string
		HouseholdIDs []string
		Time         *common.BegemotTime
		Date         *common.BegemotDate
		ExpectedOk   bool
		ExpectedNLG  libnlg.NLG
	}{
		{
			Name:         "household",
			HouseholdIDs: []string{"household-1"},
			DeviceIDs:    []string{"device-1"},
			ExpectedOk:   true,
			ExpectedNLG:  nil,
		},
		{
			Name:      "time",
			DeviceIDs: []string{"device-1"},
			Time: &common.BegemotTime{
				Hours: ptr.Int(3),
			},
			ExpectedOk:  true,
			ExpectedNLG: nil,
		},
		{
			Name:      "date",
			DeviceIDs: []string{"device-1"},
			Date: &common.BegemotDate{
				Days:         ptr.Int(3),
				DaysRelative: true,
			},
			ExpectedOk:  false,
			ExpectedNLG: nlg.NoTimeSpecified,
		},
		{
			Name:      "no_devices",
			DeviceIDs: []string{},
			Date: &common.BegemotDate{
				Days:         ptr.Int(3),
				DaysRelative: true,
			},
			ExpectedOk:  false,
			ExpectedNLG: nlg.CannotFindDevices,
		},
	}

	userInfo := model.UserInfo{
		Devices: model.Devices{
			{
				ID:          "device-1",
				HouseholdID: "household-1",
			},
			{
				ID:          "device-2",
				HouseholdID: "household-2",
			},
		},
	}
	processorContext := common.RunProcessorContext{
		Context:  suite.ctx,
		UserInfo: userInfo,
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			actionFrame := frames.ActionFrameBuilder{}
			actionFrame.WithSemanticFrame(libmegamind.SemanticFrame{
				Frame: &commonpb.TSemanticFrame{
					Name: string(frames.ActionCapabilityOnOffFrameName),
				},
			}).WithTime(common.BegemotTime{}).WithDate(common.BegemotDate{})

			if input.DeviceIDs != nil {
				actionFrame.WithDeviceIDs(input.DeviceIDs...)
			}
			if input.HouseholdIDs != nil {
				actionFrame.WithHouseholdIDs(input.HouseholdIDs...)
			}
			if input.Time != nil {
				actionFrame.WithTime(*input.Time)
			}
			if input.Date != nil {
				actionFrame.WithDate(*input.Date)
			}

			frame := actionFrame.Build()

			var ok bool
			var response *scenarios.TScenarioRunResponse
			suite.NotPanics(func() {
				ok, response, _ = suite.actionProcessor.Validate(processorContext, &frame)
			})

			suite.Equal(input.ExpectedOk, ok)
			if input.ExpectedOk {
				return
			}

			suite.True(runResponseHasCorrectNLG(response, input.ExpectedNLG))
		})
	}
}

func runResponseHasCorrectNLG(response *scenarios.TScenarioRunResponse, expectedNLG libnlg.NLG) bool {
	for _, card := range response.GetResponseBody().GetLayout().GetCards() {
		for _, asset := range expectedNLG {
			if asset.Text() == card.GetText() {
				return true
			}
		}
	}

	return false
}
