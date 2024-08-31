package queue

import (
	"context"
	"sync"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Worker struct {
	logger                log.Logger
	config                WorkerConfig
	queue                 *Queue
	broker                Broker
	taskName              string
	stopChan              chan bool
	subProcessesWaitGroup *sync.WaitGroup
	tasksInChan           uint32
	stats                 WorkerStats
	tasksChan             chan Task
	successfulTasks       chan Task
	failedTasks           chan Task
	resubmittedTasks      chan Task
	lastCheckForLostTasks time.Time
}

type WorkerStats struct {
	lastCollectedTime time.Time
	tasksFetched      int

	TasksFetchSpeed float64
}

func newWorker(taskName string, config WorkerConfig, logger log.Logger, queue *Queue, broker Broker) *Worker {
	w := &Worker{
		taskName:              taskName,
		config:                config,
		logger:                logger,
		stopChan:              make(chan bool),
		subProcessesWaitGroup: &sync.WaitGroup{},
		queue:                 queue,
		broker:                broker,
		tasksChan:             make(chan Task, config.TasksChunkSize),
		successfulTasks:       make(chan Task, 2*config.TasksChunkSize),
		failedTasks:           make(chan Task, 2*config.TasksChunkSize),
		resubmittedTasks:      make(chan Task, 2*config.TasksChunkSize),
	}

	return w
}

func (w *Worker) Launch() error {
	w.stats.lastCollectedTime = time.Now()

	subprocessFunctions := []func(){
		w.heartbeatBroker,
		w.fetchTasksFromBroker,
		w.submitProcessedTasks,
		w.oldTasksCleanup,
	}
	w.subProcessesWaitGroup.Add(len(subprocessFunctions))
	for _, f := range subprocessFunctions {
		go f()
	}

	subWorkersWaitGroup := sync.WaitGroup{}

	for i := 0; i < w.config.ProcessNum; i++ {
		subWorkersWaitGroup.Add(1)
		go func() {
			defer subWorkersWaitGroup.Done()

			for task := range w.tasksChan {
				atomic.AddUint32(&w.tasksInChan, ^uint32(0)) // decrement w.tasksInChan
				err := w.processTask(task)
				if err != nil {
					w.logger.Errorf("Task processing unexpected error: %v", err)
				}
			}
		}()
	}

	w.logger.Debug("Waiting goroutines to finish processing tasks")
	subWorkersWaitGroup.Wait()
	w.logger.Debug("Tasks processing stopped")

	close(w.failedTasks)
	close(w.successfulTasks)
	close(w.resubmittedTasks)

	ctx, cancel := context.WithTimeout(context.Background(), w.config.DatabaseExecutionTimeout)
	defer cancel()
	w.syncProcessedTasksOnStop(ctx)

	w.subProcessesWaitGroup.Wait()
	return nil
}

func (w *Worker) heartbeatBroker() {
	ctx, cancel := context.WithCancel(context.Background())
	w.broker.Heartbeat(ctx)

	ticker := time.NewTicker(w.config.HeartbeatPeriod)

	defer func() {
		w.logger.Debug("Heartbeat stopped")
		ticker.Stop()
		cancel()
		w.subProcessesWaitGroup.Done()
	}()

	for {
		select {
		case <-ticker.C:
			w.broker.Heartbeat(ctx)
		case <-w.stopChan:
			return
		}
	}
}

func (w *Worker) oldTasksCleanup() {
	ctx, cancel := context.WithCancel(context.Background())

	ticker := time.NewTicker(w.config.OldTasksCleanupPeriod)

	defer func() {
		w.logger.Debug("Periodic cleanup stopped")
		ticker.Stop()
		cancel()
		w.subProcessesWaitGroup.Done()
	}()

	for {
		select {
		case <-ticker.C:
			tasksRemoved, err := w.broker.RemoveOldFinishedTasks(ctx, w.taskName, w.config.OldTasksCleanupThreshold, w.config.OldTasksCleanupLimit)
			if err != nil {
				w.logger.Warnf("Old tasks cleanup failed: %v", err)
			} else {
				w.logger.Debugf("Removed %d old tasks", tasksRemoved)
			}
		case <-w.stopChan:
			return
		}
	}
}

func (w *Worker) fetchTasksFromBroker() {
	ctx, cancel := context.WithCancel(context.Background())
	ticker := time.NewTicker(w.config.TasksFetchPeriod)
	lastCheckForLostTasks := time.Now()
	isLostTaskFound := false

	defer func() {
		w.logger.Debug("Tasks fetch stopped")
		ticker.Stop()
		cancel()
		w.subProcessesWaitGroup.Done()
	}()

	for {
		select {
		case <-ticker.C:
			numTasksToFetch := w.config.TasksChunkSize - int(w.tasksInChan)
			if numTasksToFetch == 0 {
				continue
			}

			var tasks []Task
			var err error
			var fetchDuration time.Duration
			fetchLostTasks := isLostTaskFound && time.Since(lastCheckForLostTasks) >= w.config.LostTasksFoundFetchPeriod ||
				time.Since(lastCheckForLostTasks) >= w.config.LostTasksFetchPeriod // if we found lost task on previous iteration, we start to check for it more often

			if fetchLostTasks {
				fetchStart := time.Now()
				tasks, err = w.broker.GetLostTasksAndUpdateState(ctx, w.taskName, numTasksToFetch)
				fetchDuration = time.Since(fetchStart)
				lastCheckForLostTasks = time.Now()
				isLostTaskFound = len(tasks) > 0
				w.logger.Debugf("Lost tasks found: %t", isLostTaskFound)
			}

			if shouldProcessLostTasks := fetchLostTasks && isLostTaskFound; !(shouldProcessLostTasks) {
				fetchStart := time.Now()
				tasks, err = w.broker.GetTasksAndUpdateState(ctx, w.taskName, numTasksToFetch)
				fetchDuration = time.Since(fetchStart)
				fetchLostTasks = false
			}

			if err != nil {
				w.logger.Errorf("Cannot get tasks from broker: %v", err)
				continue
			}
			w.logger.Debugf("Fetched %d tasks from database (took %.3f seconds) - is lost: %v", len(tasks), fetchDuration.Seconds(), fetchLostTasks)

			for _, t := range tasks {
				w.tasksChan <- t
				atomic.AddUint32(&w.tasksInChan, 1)
			}

			w.stats.tasksFetched += len(tasks)
			if time.Since(w.stats.lastCollectedTime) > w.config.StatisticsCollectionPeriod {
				w.stats.TasksFetchSpeed = float64(w.stats.tasksFetched) / time.Since(w.stats.lastCollectedTime).Seconds()
				w.logger.Debugf("Fetch speed is %.2f tasks/sec", w.stats.TasksFetchSpeed)

				w.stats.tasksFetched = 0
				w.stats.lastCollectedTime = time.Now()
			}
		case <-w.stopChan:
			close(w.tasksChan)

			if w.tasksInChan > 0 {
				unprocessedTasks := make([]Task, 0, w.tasksInChan)
				for t := range w.tasksChan {
					unprocessedTasks = append(unprocessedTasks, t)
				}

				w.logger.Debugf("Returning %d tasks to database", len(unprocessedTasks))
				ctx, cancel := context.WithTimeout(context.Background(), w.config.DatabaseExecutionTimeout)
				err := w.broker.SetTasksReady(ctx, unprocessedTasks)
				cancel()
				if err != nil {
					w.logger.Errorf("Can't return %d tasks to database: %v", len(unprocessedTasks), err)
				}
			}

			return
		}
	}
}

func (w *Worker) submitProcessedTasks() {
	ctx, cancel := context.WithCancel(context.Background())
	ticker := time.NewTicker(w.config.TasksFetchPeriod)

	defer func() {
		w.logger.Debug("Submit processed tasks is stopped")
		ticker.Stop()
		cancel()
		w.subProcessesWaitGroup.Done()
	}()

	successfulTasksChunk := make([]Task, 0, w.config.TasksChunkSize)
	failedTasksChunk := make([]Task, 0, w.config.TasksChunkSize)
	resubmittedTasksChunk := make([]Task, 0, w.config.TasksChunkSize)

	syncToBroker := func() {
		w.submitTasksChunk(ctx, successfulTasksChunk, failedTasksChunk, resubmittedTasksChunk)
		successfulTasksChunk = make([]Task, 0, w.config.TasksChunkSize)
		failedTasksChunk = make([]Task, 0, w.config.TasksChunkSize)
		resubmittedTasksChunk = make([]Task, 0, w.config.TasksChunkSize)
	}

	for {
		select {
		case t, ok := <-w.successfulTasks:
			if ok {
				successfulTasksChunk = append(successfulTasksChunk, t)
			}
		case t, ok := <-w.failedTasks:
			if ok {
				failedTasksChunk = append(failedTasksChunk, t)
			}
		case t, ok := <-w.resubmittedTasks:
			if ok {
				resubmittedTasksChunk = append(resubmittedTasksChunk, t)
			}
		case <-ticker.C:
			syncToBroker()
		case <-w.stopChan:
			syncToBroker()
			return
		}
	}
}

func (w *Worker) submitTasksChunk(ctx context.Context, successful, failed, resubmitted []Task) {
	if len(successful) > 0 {
		err := w.broker.SetTasksDone(ctx, successful)
		if err != nil {
			w.logger.Errorf("Can't set done for tasks: %v", err)
		}
	}
	if len(failed) > 0 {
		err := w.broker.SetTasksFailed(ctx, failed)
		if err != nil {
			w.logger.Errorf("Can't set failed for tasks: %v", err)
		}
	}
	if len(resubmitted) > 0 {
		tasksByMergePolicy := make(map[MergePolicy][]Task)
		for _, t := range resubmitted {
			mergePolicy := w.queue.GetTaskMergePolicy(t)
			tasksByMergePolicy[mergePolicy] = append(tasksByMergePolicy[mergePolicy], t)
		}

		for policy, tasks := range tasksByMergePolicy {
			newTasks := make([]Task, len(tasks))
			copy(newTasks, tasks)
			for i := 0; i < len(newTasks); i++ {
				newTasks[i].ID = GenerateTaskID()
				newTasks[i].State = Ready
				newTasks[i].LastError = nil
				newTasks[i].RetryLeft = w.queue.GetTaskInitialRetryCount(newTasks[i].Name)
			}

			err := w.broker.SetTasksFinishedAndSubmit(ctx, tasks, newTasks, policy)
			if err != nil {
				w.logger.Errorf("Can't resubmit tasks: %v", err)
			}
		}
	}
}

func (w *Worker) syncProcessedTasksOnStop(ctx context.Context) {
	successfulTasksChunk := make([]Task, 0, w.config.TasksChunkSize)
	failedTasksChunk := make([]Task, 0, w.config.TasksChunkSize)
	resubmittedTasksChunk := make([]Task, 0, w.config.TasksChunkSize)

	for t := range w.successfulTasks {
		successfulTasksChunk = append(successfulTasksChunk, t)
	}
	for t := range w.failedTasks {
		failedTasksChunk = append(failedTasksChunk, t)
	}
	for t := range w.resubmittedTasks {
		resubmittedTasksChunk = append(resubmittedTasksChunk, t)
	}

	w.submitTasksChunk(ctx, successfulTasksChunk, failedTasksChunk, resubmittedTasksChunk)
}

func (w *Worker) Stop() {
	close(w.stopChan)
	w.logger.Debug("Signal to stop is sent")

	ctx, cancel := context.WithTimeout(context.Background(), w.config.ShutdownWaitPeriod)
	defer cancel()
	w.broker.Cleanup(ctx)
}

func (w *Worker) processTask(task Task) error {
	if _, ok := w.queue.tasks[task.Name]; !ok {
		err := xerrors.Errorf("Task with name `%s` is not registered for this worker", task.Name)
		return err
	}
	w.logger.Debugf("Process task started: %s", task.Name)

	taskCtx := ctxlog.WithFields(context.Background(),
		log.String("request_id", task.ID),
		log.String("task_name", task.Name),
		log.String("task_group_key", task.GroupKey))
	if task.Timeout > 0 {
		var cancel func()
		taskCtx, cancel = context.WithTimeout(taskCtx, task.Timeout)
		defer cancel()
	}

	overdue := time.Since(task.ScheduledTime)
	startTime := time.Now()
	taskErr := w.queue.callHandler(taskCtx, task, overdue)
	processTime := time.Since(startTime)

	if registry := w.config.Registry; registry != nil {
		taskSignals := registry.GetTaskSignals(task.Name)
		taskSignals.ProcessedTasks.Inc()
		taskSignals.OverdueTime.Add(overdue.Milliseconds())
		taskSignals.OverdueTimer.RecordDuration(overdue)
		taskSignals.ProcessTime.Add(processTime.Milliseconds())
		taskSignals.ProcessTimer.RecordDuration(processTime)
	}

	if taskErr != nil {
		if ok := xerrors.Is(taskErr, &PanicOnTaskError{}); ok {
			if registry := w.config.Registry; registry != nil {
				taskSignals := registry.GetTaskSignals(task.Name)
				taskSignals.TasksPanicked.Inc()
			}
		}

		var doneErr DoneAndResubmitTaskError
		if ok := xerrors.As(taskErr, &doneErr); ok {
			w.logger.Debugf("Process task finished: %s (process time: %f sec, overdue: %f sec); resubmitting in %.2f sec",
				task.Name, processTime.Seconds(), overdue.Seconds(), doneErr.delay.Seconds())
			w.setTaskDoneAndResubmit(task, doneErr.delay)
			return nil
		}

		var failErr FailAndResubmitTaskError
		if ok := xerrors.As(taskErr, &failErr); ok {
			w.logger.Debugf("Process task finished: %s (process time: %f sec, overdue: %f sec); resubmitting in %.2f sec",
				task.Name, processTime.Seconds(), overdue.Seconds(), failErr.delay.Seconds())
			w.setTaskFailedAndResubmit(task, failErr.err, failErr.delay)
			return nil
		}

		w.logger.Errorf("Task process finished with error: %v (process time: %f sec, overdue: %f sec)",
			taskErr, processTime.Seconds(), overdue.Seconds())
		delay := w.queue.GetTaskDelay(task)
		w.setTaskFailed(task, taskErr, delay)

		return nil
	}

	w.logger.Debugf("Process task finished: %s (process time: %f sec, overdue: %f sec)",
		task.Name, processTime.Seconds(), overdue.Seconds())
	w.setTaskDone(task)
	return nil
}

func (w *Worker) setTaskDone(t Task) {
	t.State = Done
	t.LastError = nil
	w.successfulTasks <- t
}

func (w *Worker) setTaskDoneAndResubmit(t Task, delay time.Duration) {
	t.State = Done
	t.LastError = nil
	t.ScheduledTime = time.Now().Add(delay)

	w.resubmittedTasks <- t
}

func (w *Worker) setTaskFailedAndResubmit(t Task, lastErr error, delay time.Duration) {
	t.State = Failed
	t.LastError = ptr.String(lastErr.Error())
	t.ScheduledTime = time.Now().Add(delay)

	w.resubmittedTasks <- t
}

func (w *Worker) setTaskFailed(t Task, lastErr error, delay time.Duration) {
	t.LastError = ptr.String(lastErr.Error())

	if t.RetryLeft == 0 {
		t.State = Failed
	} else {
		t.RetryLeft -= 1
		t.State = Ready
		t.ScheduledTime = time.Now().Add(delay).UTC()
	}
	w.failedTasks <- t
}
