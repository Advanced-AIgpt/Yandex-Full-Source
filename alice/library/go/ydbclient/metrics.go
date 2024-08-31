package ydbclient

import (
	"context"
	"runtime/debug"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func NewYDBClientWithMetrics(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, registry metrics.Registry, period time.Duration, options ...Options) (*YDBClient, func(ctx context.Context), error) {
	s := newSignals(registry)

	var opt Options
	switch len(options) {
	case 0: // pass
	case 1:
		opt = options[0]
	default:
		return nil, nil, xerrors.Errorf("failed to create ydb client: too many options received: %v", len(options))
	}

	if opt.TableTracer == nil {
		opt.TableTracer = &table.ClientTrace{}
	}

	opt.TableTracer.OnCreateSession = func(info table.CreateSessionStartInfo) func(table.CreateSessionDoneInfo) {
		return func(info table.CreateSessionDoneInfo) {
			if info.Error == nil {
				s.PoolBalance.Add(1)
			}
		}
	}

	opt.TableTracer.OnDeleteSession = func(info table.DeleteSessionStartInfo) func(table.DeleteSessionDoneInfo) {
		return func(info table.DeleteSessionDoneInfo) {
			if info.Error == nil {
				s.PoolBalance.Add(-1)
			}
		}
	}

	ydbClient, err := NewYDBClient(ctx, logger, endpoint, prefix, credentials, trace, opt)
	if err != nil {
		return nil, nil, err
	}

	ticker := time.NewTicker(period)

	go func() {
		defer func() {
			if r := recover(); r != nil {
				logger.Warn("caught panic in YDB CollectMetrics", log.Any("stacktrace", string(debug.Stack())))
			}
		}()

		for range ticker.C {
			stats := ydbClient.SessionPool.Stats()
			s.IdleSessions.Set(float64(stats.Idle))
			s.ReadySessions.Set(float64(stats.Ready))
			s.IndexSessions.Set(float64(stats.Index))
			s.WaitQSessions.Set(float64(stats.WaitQ))
			s.MinSessions.Set(float64(stats.MinSize))
			s.MaxSessions.Set(float64(stats.MaxSize))
			s.CreateInProgressSessions.Set(float64(stats.CreateInProgress))
			s.BusyCheckSessions.Set(float64(stats.BusyCheck))
		}
	}()

	stopFunc := func(_ context.Context) {
		ticker.Stop()
	}
	return ydbClient, stopFunc, nil
}

type signals struct {
	IdleSessions             metrics.Gauge
	ReadySessions            metrics.Gauge
	IndexSessions            metrics.Gauge
	WaitQSessions            metrics.Gauge
	MinSessions              metrics.Gauge
	MaxSessions              metrics.Gauge
	CreateInProgressSessions metrics.Gauge
	BusyCheckSessions        metrics.Gauge

	PoolBalance metrics.Counter
}

func newSignals(registry metrics.Registry) *signals {
	statsRegistry := registry.WithPrefix("stats")
	s := &signals{
		IdleSessions:             statsRegistry.Gauge("idle_sessions"),
		ReadySessions:            statsRegistry.Gauge("ready_sessions"),
		IndexSessions:            statsRegistry.Gauge("index_sessions"),
		WaitQSessions:            statsRegistry.Gauge("waitq_sessions"),
		MinSessions:              statsRegistry.Gauge("min_sessions"),
		MaxSessions:              statsRegistry.Gauge("max_sessions"),
		CreateInProgressSessions: statsRegistry.Gauge("create_in_progress_sessions"),
		BusyCheckSessions:        statsRegistry.Gauge("busy_check_sessions"),

		PoolBalance: statsRegistry.Counter("pool_balance"),
	}
	return s
}
