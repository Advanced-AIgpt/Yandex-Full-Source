package pgbroker

import (
	"context"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BrokerWithMetrics struct {
	broker *Broker

	logger log.Logger

	queueRegistry *queue.SignalsRegistry
	timer         *time.Ticker

	dbRegistry                        metrics.Registry
	submitTasksSignals                quasarmetrics.PGDBSignals
	getTasksAndUpdateStateSignals     quasarmetrics.PGDBSignals
	getLostTasksAndUpdateStateSignals quasarmetrics.PGDBSignals
	setTasksReadySignals              quasarmetrics.PGDBSignals
	setTasksDoneSignals               quasarmetrics.PGDBSignals
	setTasksDoneAndSubmitSignals      quasarmetrics.PGDBSignals
	setTasksFailedSignals             quasarmetrics.PGDBSignals
	removeOldFinishedTasksSignals     quasarmetrics.PGDBSignals
}

type DBMetrics struct {
	Registry metrics.Registry
	Policy   quasarmetrics.BucketsGenerationPolicy
}

type QueueMetrics struct {
	Registry     *queue.SignalsRegistry
	GatherPeriod time.Duration
}

func NewBrokerWithMetrics(logger log.Logger, pgClient *pgclient.PGClient, dbMetrics DBMetrics, queueMetrics QueueMetrics) *BrokerWithMetrics {
	return &BrokerWithMetrics{
		broker: &Broker{
			dbClient: pgClient,
		},

		logger: logger,

		queueRegistry: queueMetrics.Registry,
		timer:         time.NewTicker(queueMetrics.GatherPeriod),

		dbRegistry:                        dbMetrics.Registry,
		submitTasksSignals:                quasarmetrics.NewPGDBSignals("submitTasks", dbMetrics.Registry, dbMetrics.Policy),
		getTasksAndUpdateStateSignals:     quasarmetrics.NewPGDBSignals("getTasksAndUpdateState", dbMetrics.Registry, dbMetrics.Policy),
		getLostTasksAndUpdateStateSignals: quasarmetrics.NewPGDBSignals("getLostTasksAndUpdateState", dbMetrics.Registry, dbMetrics.Policy),
		setTasksReadySignals:              quasarmetrics.NewPGDBSignals("setTasksReady", dbMetrics.Registry, dbMetrics.Policy),
		setTasksDoneSignals:               quasarmetrics.NewPGDBSignals("setTasksDone", dbMetrics.Registry, dbMetrics.Policy),
		setTasksDoneAndSubmitSignals:      quasarmetrics.NewPGDBSignals("setTasksDoneAndSubmit", dbMetrics.Registry, dbMetrics.Policy),
		setTasksFailedSignals:             quasarmetrics.NewPGDBSignals("setTasksFailed", dbMetrics.Registry, dbMetrics.Policy),
		removeOldFinishedTasksSignals:     quasarmetrics.NewPGDBSignals("removeOldFinishedTasks", dbMetrics.Registry, dbMetrics.Policy),
	}
}

func (b *BrokerWithMetrics) SubmitTasks(ctx context.Context, tasks []queue.Task, mergePolicy queue.MergePolicy) error {
	start := time.Now()
	defer b.submitTasksSignals.RecordDurationSince(start)

	err := b.broker.SubmitTasks(ctx, tasks, mergePolicy)

	b.submitTasksSignals.RecordMetrics(err)
	return err
}
func (b *BrokerWithMetrics) GetTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]queue.Task, error) {
	start := time.Now()
	defer b.getTasksAndUpdateStateSignals.RecordDurationSince(start)

	tasks, err := b.broker.GetTasksAndUpdateState(ctx, taskName, limit)

	b.getTasksAndUpdateStateSignals.RecordMetrics(err)
	return tasks, err
}
func (b *BrokerWithMetrics) GetLostTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]queue.Task, error) {
	start := time.Now()
	defer b.getLostTasksAndUpdateStateSignals.RecordDurationSince(start)

	tasks, err := b.broker.GetLostTasksAndUpdateState(ctx, taskName, limit)

	b.getLostTasksAndUpdateStateSignals.RecordMetrics(err)
	return tasks, err
}
func (b *BrokerWithMetrics) SetTasksReady(ctx context.Context, tasks []queue.Task) error {
	start := time.Now()
	defer b.setTasksReadySignals.RecordDurationSince(start)

	err := b.broker.SetTasksReady(ctx, tasks)

	b.setTasksReadySignals.RecordMetrics(err)
	return err
}
func (b *BrokerWithMetrics) SetTasksDone(ctx context.Context, tasks []queue.Task) error {
	start := time.Now()
	defer b.setTasksDoneSignals.RecordDurationSince(start)

	err := b.broker.SetTasksDone(ctx, tasks)

	b.setTasksDoneSignals.RecordMetrics(err)
	return err
}
func (b *BrokerWithMetrics) SetTasksFinishedAndSubmit(ctx context.Context, tasksDone []queue.Task, tasksSubmit []queue.Task, mergePolicy queue.MergePolicy) error {
	start := time.Now()
	defer b.setTasksDoneAndSubmitSignals.RecordDurationSince(start)

	err := b.broker.SetTasksFinishedAndSubmit(ctx, tasksDone, tasksSubmit, mergePolicy)

	b.setTasksDoneAndSubmitSignals.RecordMetrics(err)
	return err
}
func (b *BrokerWithMetrics) SetTasksFailed(ctx context.Context, tasks []queue.Task) error {
	start := time.Now()
	defer b.setTasksFailedSignals.RecordDurationSince(start)

	err := b.broker.SetTasksFailed(ctx, tasks)

	b.setTasksFailedSignals.RecordMetrics(err)
	return err
}
func (b *BrokerWithMetrics) RemoveOldFinishedTasks(ctx context.Context, taskName string, threshold time.Duration, limit int) (int, error) {
	start := time.Now()
	defer b.removeOldFinishedTasksSignals.RecordDurationSince(start)

	removedNum, err := b.broker.RemoveOldFinishedTasks(ctx, taskName, threshold, limit)

	b.removeOldFinishedTasksSignals.RecordMetrics(err)
	return removedNum, err
}
func (b *BrokerWithMetrics) Heartbeat(ctx context.Context) {
	b.broker.Heartbeat(ctx)
}
func (b *BrokerWithMetrics) Cleanup(ctx context.Context) {
	b.broker.Cleanup(ctx)
}

func (b *BrokerWithMetrics) collectTaskMetrics(ctx context.Context) error {
	query := `
		SELECT
			name, state, count(*)
		FROM
			iot.tasks
		GROUP BY
			name, state
	`

	db, err := b.broker.dbClient.GetSecondaryPreferred()
	if err != nil {
		return err
	}

	rows, err := db.QueryContext(ctx, query)
	if err != nil {
		return err
	}
	defer func() {
		_ = rows.Close()
	}()

	type taskRow struct {
		name  string
		state queue.State
		count int
	}
	type taskStatusCount struct {
		readyCount   int
		runningCount int
		doneCount    int
		failedCount  int
	}
	taskToCountMap := make(map[string]taskStatusCount)
	for rows.Next() {
		var tr taskRow
		if err := rows.Scan(&tr.name, &tr.state, &tr.count); err != nil {
			return err
		}
		statusCount := taskToCountMap[tr.name]
		switch tr.state {
		case queue.Ready:
			statusCount.readyCount = tr.count
		case queue.Running:
			statusCount.runningCount = tr.count
		case queue.Done:
			statusCount.doneCount = tr.count
		case queue.Failed:
			statusCount.failedCount = tr.count
		default:
			return xerrors.Errorf("unknown task %s state: %s", tr.name, tr.state)
		}
		taskToCountMap[tr.name] = statusCount
	}
	for taskName, statusCount := range taskToCountMap {
		taskSignals := b.queueRegistry.GetTaskSignals(taskName)
		taskSignals.ReadyTasks.Set(float64(statusCount.readyCount))
		taskSignals.RunningTasks.Set(float64(statusCount.runningCount))
		taskSignals.DoneTasks.Set(float64(statusCount.doneCount))
		taskSignals.FailedTasks.Set(float64(statusCount.failedCount))
	}
	return nil
}

func (b *BrokerWithMetrics) Launch() {
	go func() {
		for range b.timer.C {
			if err := b.collectTaskMetrics(context.Background()); err != nil {
				b.logger.Warnf("broker metrics collection error: %s", err)
			}
		}
	}()
}

func (b *BrokerWithMetrics) Stop() {
	b.timer.Stop()
}
