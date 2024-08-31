package product

import (
	"time"

	"go.uber.org/zap"

	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/library/go/core/metrics"
)

const (
	_updateTime = 20 * time.Minute
)

type totalUsers struct {
	lastUpdate   time.Time
	lastResponse float64
}

type activeUsers struct {
	lastUpdate  time.Time
	lastDaily   float64
	lastWeekly  float64
	lastMonthly float64
}

type Metrics struct {
	storage       session.Storage
	totalUsers    totalUsers
	activeUsers   activeUsers
	logger        *zap.SugaredLogger
	errorsCounter metrics.Counter
}

func NewMetrics(storage session.Storage, logger *zap.SugaredLogger) *Metrics {
	return &Metrics{
		storage: storage,
		logger:  logger,
	}
}

func (m *Metrics) RegisterMetricsHandlers(registry metrics.Registry) {
	registry = registry.WithPrefix(sensors.ProductPrefix)
	m.errorsCounter = sensors.MakeRatedCounter(registry, sensors.ErrorsCountPerSecond)
	registry.FuncGauge(sensors.TotalUsers, m.onTotalUsers)
}

func (m *Metrics) onTotalUsers() float64 {
	if time.Since(m.totalUsers.lastUpdate) > _updateTime {
		total, err := m.storage.Count()
		if err != nil {
			m.logger.Errorf("unable to update total users: %w", err)
			m.errorsCounter.Inc()
		} else {
			m.totalUsers.lastResponse = float64(total)
			m.totalUsers.lastUpdate = time.Now()
		}
	}
	return m.totalUsers.lastResponse
}
