package model

import (
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCanCreateStereopair(t *testing.T) {
	type testDescription struct {
		name           string
		leaderDevice   Device
		followerDevice Device
		stereopairs    Stereopairs
		result         error
	}

	testErr := errors.New("some error")

	input := []testDescription{
		{
			name:           "simple_ok",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationDeviceType),
			stereopairs:    nil,
			result:         nil,
		},
		{
			name:           "mini2_with_and_without_clock",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2DeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2NoClockDeviceType),
			stereopairs:    nil,
			result:         nil,
		},
		{
			name:           "mini2_without_clock_and_with",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs:    nil,
			result:         nil,
		},
		{
			name:           "cant_same_id",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs:    nil,
			result:         testErr,
		},
		{
			name:           "leader_is_leader_in_other_stereopair",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs: Stereopairs{
				Stereopair{Config: StereopairConfig{
					Devices: StereopairDeviceConfigs{
						StereopairDeviceConfig{
							ID:      "1",
							Channel: LeftChannel,
							Role:    LeaderRole,
						},
						StereopairDeviceConfig{
							ID:      "other",
							Channel: RightChannel,
							Role:    FollowerRole,
						},
					},
				},
				},
			},
			result: testErr,
		},
		{
			name:           "leader_is_follower_in_other_stereopair",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs: Stereopairs{
				Stereopair{Config: StereopairConfig{
					Devices: StereopairDeviceConfigs{
						StereopairDeviceConfig{
							ID:      "1",
							Channel: LeftChannel,
							Role:    FollowerRole,
						},
						StereopairDeviceConfig{
							ID:      "other",
							Channel: RightChannel,
							Role:    LeaderRole,
						},
					},
				},
				},
			},
			result: testErr,
		},
		{
			name:           "follower_is_leader_in_other_stereopair",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs: Stereopairs{
				Stereopair{Config: StereopairConfig{
					Devices: StereopairDeviceConfigs{
						StereopairDeviceConfig{
							ID:      "2",
							Channel: LeftChannel,
							Role:    LeaderRole,
						},
						StereopairDeviceConfig{
							ID:      "other",
							Channel: RightChannel,
							Role:    FollowerRole,
						},
					},
				},
				},
			},
			result: testErr,
		},
		{
			name:           "follower_is_follower_in_other_stereopair",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationMini2NoClockDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationMini2DeviceType),
			stereopairs: Stereopairs{
				Stereopair{Config: StereopairConfig{
					Devices: StereopairDeviceConfigs{
						StereopairDeviceConfig{
							ID:      "2",
							Channel: LeftChannel,
							Role:    FollowerRole,
						},
						StereopairDeviceConfig{
							ID:      "other",
							Channel: RightChannel,
							Role:    LeaderRole,
						},
					},
				},
				},
			},
			result: testErr,
		},
		{
			name:           "leader_stereopair_not_available",
			leaderDevice:   *NewDevice("l").WithID("1").WithDeviceType(OtherDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationDeviceType),
			stereopairs:    nil,
			result:         testErr,
		},
		{
			name:           "leader_stereopair_not_available",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationDeviceType),
			followerDevice: *NewDevice("f").WithID("2").WithDeviceType(OtherDeviceType),
			stereopairs:    nil,
			result:         testErr,
		},
		{
			name:           "incompatible_device_types",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationDeviceType),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStation2DeviceType),
			stereopairs:    nil,
			result:         testErr,
		},
		{
			name:           "different_houses",
			leaderDevice:   *NewDevice("l").WithSkillID(QUASAR).WithID("1").WithDeviceType(YandexStationDeviceType).WithHouseholdID("1"),
			followerDevice: *NewDevice("f").WithSkillID(QUASAR).WithID("2").WithDeviceType(YandexStationDeviceType).WithHouseholdID("2"),
			stereopairs:    nil,
			result:         testErr,
		},
	}

	// test about all known stereopair devices can pair with same device type
	for leaderDeviceType := range StereopairAvailablePairs {
		input = append(input, testDescription{
			name:           "test_generated_same_" + leaderDeviceType.String(),
			leaderDevice:   *NewDevice("leader").WithSkillID(QUASAR).WithID("1").WithDeviceType(leaderDeviceType),
			followerDevice: *NewDevice("follower").WithSkillID(QUASAR).WithID("2").WithDeviceType(leaderDeviceType),
			stereopairs:    nil,
			result:         nil,
		})
	}

	for _, testCase := range input {
		t.Run(testCase.name, func(t *testing.T) {
			at := assert.New(t)
			res := CanCreateStereopair(testCase.leaderDevice, testCase.followerDevice, testCase.stereopairs)
			switch testCase.result {
			case nil:
				at.NoError(res)
			case testErr:
				at.Error(res)
			default:
				at.ErrorIs(res, testCase.result)
			}
		})
	}
}
