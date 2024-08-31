package db

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ClientWithMetrics struct {
	client   DB
	registry metrics.Registry

	// user
	getTuyaUserSignals        quasarmetrics.YDBSignals
	createUserSignals         quasarmetrics.YDBSignals
	isKnownUserSignals        quasarmetrics.YDBSignals
	getTuyaUserSkillIDSignals quasarmetrics.YDBSignals

	// device owner
	getDeviceOwnerSignals        quasarmetrics.YDBSignals
	setDevicesOwnerSignals       quasarmetrics.YDBSignals
	invalidateDeviceOwnerSignals quasarmetrics.YDBSignals

	// custom controls
	selectCustomControlSignals     quasarmetrics.YDBSignals
	selectUserCustomControlSignals quasarmetrics.YDBSignals
	storeCustomControlSignals      quasarmetrics.YDBSignals
	deleteCustomControlSignals     quasarmetrics.YDBSignals
}

func NewMetricsClientWithDB(client DB, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ClientWithMetrics {
	return &ClientWithMetrics{
		client:   client,
		registry: registry,

		// user
		createUserSignals:         quasarmetrics.NewYDBSignals("createUser", registry, policy),
		getTuyaUserSignals:        quasarmetrics.NewYDBSignals("getTuyaUser", registry, policy),
		isKnownUserSignals:        quasarmetrics.NewYDBSignals("isKnownUser", registry, policy),
		getTuyaUserSkillIDSignals: quasarmetrics.NewYDBSignals("getTuyaUserSkillID", registry, policy),

		// device owner
		getDeviceOwnerSignals:        quasarmetrics.NewYDBSignals("getDeviceOwner", registry, policy),
		setDevicesOwnerSignals:       quasarmetrics.NewYDBSignals("setDevicesOwner", registry, policy),
		invalidateDeviceOwnerSignals: quasarmetrics.NewYDBSignals("invalidateDeviceOwner", registry, policy),

		// presets
		selectCustomControlSignals:     quasarmetrics.NewYDBSignals("selectCustomControl", registry, policy),
		selectUserCustomControlSignals: quasarmetrics.NewYDBSignals("selectUserCustomControl", registry, policy),
		storeCustomControlSignals:      quasarmetrics.NewYDBSignals("storeCustomControl", registry, policy),
		deleteCustomControlSignals:     quasarmetrics.NewYDBSignals("deleteCustomControl", registry, policy),
	}
}

func (m *ClientWithMetrics) GetTuyaUserID(ctx context.Context, userID uint64, skillID string) (string, error) {
	start := time.Now()
	defer m.getTuyaUserSignals.RecordDurationSince(start)

	tuyaUserID, err := m.client.GetTuyaUserID(ctx, userID, skillID)

	m.getTuyaUserSignals.RecordMetrics(err)
	return tuyaUserID, err
}

func (m *ClientWithMetrics) CreateUser(ctx context.Context, userID uint64, skillID, login, tuyaUID string) error {
	start := time.Now()
	defer m.createUserSignals.RecordDurationSince(start)

	err := m.client.CreateUser(ctx, userID, skillID, login, tuyaUID)

	m.createUserSignals.RecordMetrics(err)
	return err
}

func (m *ClientWithMetrics) IsKnownUser(ctx context.Context, tuyaUID string) (bool, error) {
	start := time.Now()
	defer m.isKnownUserSignals.RecordDurationSince(start)

	isKnown, err := m.client.IsKnownUser(ctx, tuyaUID)

	m.isKnownUserSignals.RecordMetrics(err)
	return isKnown, err
}

func (m *ClientWithMetrics) GetTuyaUserSkillID(ctx context.Context, tuyaUID string) (string, error) {
	start := time.Now()
	defer m.getTuyaUserSkillIDSignals.RecordDurationSince(start)

	skillID, err := m.client.GetTuyaUserSkillID(ctx, tuyaUID)

	m.getTuyaUserSkillIDSignals.RecordMetrics(err)
	return skillID, err
}

func (m *ClientWithMetrics) GetDeviceOwner(ctx context.Context, deviceID string, maxAge time.Duration) (tuya.DeviceOwner, error) {
	start := time.Now()
	defer m.getDeviceOwnerSignals.RecordDurationSince(start)

	deviceOwner, err := m.client.GetDeviceOwner(ctx, deviceID, maxAge)

	m.getDeviceOwnerSignals.RecordMetrics(err)
	return deviceOwner, err
}

func (m *ClientWithMetrics) SetDevicesOwner(ctx context.Context, deviceIDs []string, owner tuya.DeviceOwner) error {
	start := time.Now()
	defer m.setDevicesOwnerSignals.RecordDurationSince(start)

	err := m.client.SetDevicesOwner(ctx, deviceIDs, owner)

	m.setDevicesOwnerSignals.RecordMetrics(err)
	return err
}

func (m *ClientWithMetrics) InvalidateDeviceOwner(ctx context.Context, deviceID string) error {
	start := time.Now()
	defer m.invalidateDeviceOwnerSignals.RecordDurationSince(start)

	err := m.client.InvalidateDeviceOwner(ctx, deviceID)

	m.invalidateDeviceOwnerSignals.RecordMetrics(err)
	return err
}

func (m *ClientWithMetrics) SelectCustomControl(ctx context.Context, userID, deviceID string) (tuya.IRCustomControl, error) {
	start := time.Now()
	defer m.selectCustomControlSignals.RecordDurationSince(start)

	preset, err := m.client.SelectCustomControl(ctx, userID, deviceID)

	m.selectCustomControlSignals.RecordMetrics(err)
	return preset, err
}

func (m *ClientWithMetrics) SelectUserCustomControls(ctx context.Context, userID string) (tuya.IRCustomControls, error) {
	start := time.Now()
	defer m.selectUserCustomControlSignals.RecordDurationSince(start)

	presets, err := m.client.SelectUserCustomControls(ctx, userID)

	m.selectUserCustomControlSignals.RecordMetrics(err)
	return presets, err
}

func (m *ClientWithMetrics) StoreCustomControl(ctx context.Context, userID string, cp tuya.IRCustomControl) error {
	start := time.Now()
	defer m.storeCustomControlSignals.RecordDurationSince(start)

	err := m.client.StoreCustomControl(ctx, userID, cp)

	m.storeCustomControlSignals.RecordMetrics(err)
	return err
}

func (m *ClientWithMetrics) DeleteCustomControl(ctx context.Context, userID, controlID string) error {
	start := time.Now()
	defer m.deleteCustomControlSignals.RecordDurationSince(start)

	err := m.client.DeleteCustomControl(ctx, userID, controlID)

	m.deleteCustomControlSignals.RecordMetrics(err)
	return err
}
