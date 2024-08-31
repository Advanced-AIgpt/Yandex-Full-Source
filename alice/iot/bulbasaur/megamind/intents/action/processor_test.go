package action

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	xtestmegamind "a.yandex-team.ru/alice/iot/bulbasaur/xtest/megamind"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
)

func TestValidateActionFrameV2(t *testing.T) {
	inputs := []struct {
		name          string
		clientInfo    libmegamind.ClientInfo
		frame         frames.ActionFrameV2
		expectedError *common.ValidationError
		createdTime   time.Time
	}{
		{
			name: "turn_on_all_devices_1",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar.app",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
				AllDevicesRequired: frames.AllDevicesRequestedSlot{
					Value: "всё",
				},
			},
			expectedError: &common.TurnOnEverythingIsForbiddenValidationError,
		},
		{
			name: "turn_on_all_devices_2",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar.app",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
				Households: frames.HouseholdSlots{
					{
						HouseholdID: "h-id",
						SlotType:    string(frames.HouseholdSlotType),
					},
				},
			},
			expectedError: &common.TurnOnEverythingIsForbiddenValidationError,
		},
		{
			name: "turn_on_all_devices_3",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar.app",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &frames.OnOffValueSlot{
					Value: true,
				},
				Rooms: frames.RoomSlots{
					{
						RoomIDs:  []string{"r-id"},
						SlotType: string(frames.RoomSlotType),
					},
				},
			},
			expectedError: &common.TurnOnEverythingIsForbiddenValidationError,
		},
		{
			name: "turn_off_all_devices",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar.app",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &frames.OnOffValueSlot{
					Value: false,
				},
				AllDevicesRequired: frames.AllDevicesRequestedSlot{
					Value: "все устройства",
				},
			},
			expectedError: nil,
		},
		{
			name: "video_stream_capability_iot_app",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "com.yandex.iot",
				DeviceID:   "phone-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.VideoStreamCapabilityType),
						CapabilityInstance: string(model.GetStreamCapabilityInstance),
					},
				},
			},
			expectedError: &common.CannotPlayVideoStreamInIotAppValidationError,
		},
		{
			name: "video_stream_capability_no_supported_protocols",
			clientInfo: libmegamind.ClientInfo{
				AppID:       "ru.yandex.quasar",
				DeviceModel: "yandexmicro",
				DeviceID:    "speaker-id",
				ClientTime:  "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.VideoStreamCapabilityType),
						CapabilityInstance: string(model.GetStreamCapabilityInstance),
					},
				},
			},
			expectedError: &common.CannotPlayVideoOnDeviceValidationError,
		},
		{
			name: "unknown_num_mode",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType: string(model.ModeCapabilityType),
					},
				},
				ModeValue: &frames.ModeValueSlot{
					ModeValue: frames.UnknownModeValue,
				},
			},
			expectedError: &common.UnknownModeValidationError,
		},
		{
			name: "date_no_time_1",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				ExactDate: frames.ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Months: ptr.Int(5),
							Days:   ptr.Int(20),
						},
					},
				},
			},
			expectedError: &common.TimeIsNotSpecifiedValidationError,
		},
		{
			name: "date_no_time_2",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				RelativeDateTime: frames.RelativeDateTimeSlot{
					DateTimeRanges: []*common.BegemotDateTimeRange{
						{
							End: &common.DateTimeRangeInternal{
								Days: ptr.Int(2),
							},
						},
					},
				},
			},
			expectedError: &common.TimeIsNotSpecifiedValidationError,
		},
		{
			name: "exact_date_and_relative_time",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				ExactDate: frames.ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Months: ptr.Int(5),
							Days:   ptr.Int(20),
						},
					},
				},
				RelativeDateTime: frames.RelativeDateTimeSlot{
					DateTimeRanges: []*common.BegemotDateTimeRange{
						{
							End: &common.DateTimeRangeInternal{
								Hours: ptr.Int(2),
							},
						},
					},
				},
			},
			expectedError: &common.WeirdTimeRelativityValidationError,
		},
		{
			name: "date_time_good",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				ExactTime: frames.ExactTimeSlot{
					Times: []*common.BegemotTime{
						{
							Hours:   ptr.Int(14),
							Minutes: ptr.Int(20),
						},
					},
				},
			},
			expectedError: nil,
		},
		{
			name: "past_action",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				ExactDate: frames.ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Years:  ptr.Int(2020),
							Months: ptr.Int(02),
							Days:   ptr.Int(1),
						},
					},
				},
				ExactTime: frames.ExactTimeSlot{
					Times: []*common.BegemotTime{
						{
							Hours: ptr.Int(0),
						},
					},
				},
				ParsedTime: time.Date(2020, 2, 1, 0, 0, 0, 0, time.UTC),
			},
			createdTime:   time.Date(2022, 2, 22, 22, 22, 22, 0, time.UTC),
			expectedError: &common.PastActionValidationError,
		},
		{
			name: "far_future",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				ExactDate: frames.ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Years:  ptr.Int(2022),
							Months: ptr.Int(02),
							Days:   ptr.Int(1),
						},
					},
				},
				ExactTime: frames.ExactTimeSlot{
					Times: []*common.BegemotTime{
						{
							Hours: ptr.Int(0),
						},
					},
				},
				ParsedTime: time.Date(2022, 2, 1, 0, 0, 0, 0, time.UTC),
			},
			createdTime:   time.Date(2022, 1, 1, 22, 22, 22, 0, time.UTC),
			expectedError: &common.FarFutureValidationError,
		},
		{
			name: "good",
			clientInfo: libmegamind.ClientInfo{
				AppID:      "ru.yandex.quasar",
				DeviceID:   "speaker-id",
				ClientTime: "20220222T022222",
			},
			frame: frames.ActionFrameV2{
				IntentParameters: frames.ActionIntentParametersSlots{
					frames.ActionIntentParametersSlot{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				RelativeDateTime: frames.RelativeDateTimeSlot{
					DateTimeRanges: []*common.BegemotDateTimeRange{
						{
							End: &common.DateTimeRangeInternal{
								Hours: ptr.Int(2),
							},
						},
					},
				},
				ParsedTime: time.Date(2022, 1, 1, 22, 22, 22, 0, time.UTC),
			},
			createdTime:   time.Date(2022, 1, 1, 20, 22, 22, 0, time.UTC),
			expectedError: nil,
		},
	}

	for _, input := range inputs {
		testTimestamper := timestamp.NewMockTimestamper()
		testTimestamper.CreatedTimestampValue = timestamp.PastTimestamp(input.createdTime.Unix())
		ctx := timestamp.ContextWithTimestamper(context.Background(), testTimestamper)
		runContext := xtestmegamind.NewRunContext(ctx, xtestlogs.NopLogger(), nil).
			WithClientInfo(common.ClientInfo{
				ClientInfo: input.clientInfo,
			})
		t.Run(input.name, func(t *testing.T) {
			var err error
			assert.NotPanics(t, func() {
				err = validateFrame(runContext, &input.frame)
			})

			if input.expectedError == nil {
				assert.NoError(t, err)
				return
			}

			assert.ErrorIs(t, err, *input.expectedError)
		})
	}
}
