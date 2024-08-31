package queue

import (
	"context"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_SimpleTasksExecution(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: tasksCount,
		TasksTimeout:     5 * time.Second,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithSleep)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(3), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(tasksCount), broker.TasksDone)
}

type Payload struct {
	Data string
}

const tasksCount = 15

type FakeBroker struct {
	TotalTasksNumber int
	TasksTimeout     time.Duration
	TasksFailed      int32
	TasksDone        int32
	TasksResubmitted int32
	taskCounter      int
}

func (b *FakeBroker) SetTasksDone(ctx context.Context, tasks []Task) error {
	atomic.AddInt32(&b.TasksDone, int32(len(tasks)))
	return nil
}

func (b *FakeBroker) SetTasksFinishedAndSubmit(ctx context.Context, finishedTasks []Task, newTasks []Task, mergePolicy MergePolicy) error {
	doneTasks := 0
	failedTasks := 0
	for _, t := range finishedTasks {
		if t.State == Done {
			doneTasks += 1
		} else if t.State == Failed {
			failedTasks += 1
		}
	}

	atomic.AddInt32(&b.TasksDone, int32(doneTasks))
	atomic.AddInt32(&b.TasksFailed, int32(failedTasks))
	atomic.AddInt32(&b.TasksResubmitted, int32(len(newTasks)))
	return nil
}

func (b *FakeBroker) SetTasksFailed(ctx context.Context, tasks []Task) error {
	atomic.AddInt32(&b.TasksFailed, int32(len(tasks)))
	return nil
}

func (b *FakeBroker) SetTasksReady(ctx context.Context, tasks []Task) error {
	return nil
}

func (b *FakeBroker) Cleanup(ctx context.Context) {
}

func (b *FakeBroker) Heartbeat(ctx context.Context) {
}

func (b *FakeBroker) SubmitTasks(ctx context.Context, tasks []Task, mergePolicy MergePolicy) error {
	return nil
}

func (b *FakeBroker) RemoveOldFinishedTasks(ctx context.Context, taskName string, threshold time.Duration, limit int) (int, error) {
	return 0, nil
}

func (b *FakeBroker) GetTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]Task, error) {
	if b.TotalTasksNumber-b.taskCounter < limit {
		limit = b.TotalTasksNumber - b.taskCounter
	}

	tasks := make([]Task, 0, limit)
	for i := 0; i < limit; i++ {
		t, err := NewTaskWithTimeout("task", "123", Payload{Data: "payload"}, time.Now(), b.TasksTimeout)
		if err != nil {
			return nil, err
		}
		tasks = append(tasks, t)
		b.taskCounter++
	}
	return tasks, nil
}

func (b *FakeBroker) GetLostTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]Task, error) {
	return []Task{}, nil
}

func taskWithSleep(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	time.Sleep(2 * time.Millisecond)
	return nil
}

func Test_TasksTimeoutExceeded(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: tasksCount,
		TasksTimeout:     10 * time.Millisecond,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithContextDoneCheck)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(3), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()

	assert.Equal(t, int32(tasksCount), broker.TasksFailed)
}

func taskWithContextDoneCheck(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	timer := time.NewTimer(100 * time.Millisecond)

	select {
	case <-ctx.Done():
		timer.Stop()
		return ctx.Err()
	case <-timer.C:
		return nil
	}
}

func Test_TaskWithError(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: 1,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithError)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(1), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(1), broker.TasksFailed)
}

func taskWithError(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	return xerrors.Errorf("some error message here")
}

func Test_TaskWithResubmit(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: tasksCount,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithResubmit)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(1), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(0), broker.TasksFailed)
	assert.Equal(t, int32(tasksCount), broker.TasksDone)
	assert.Equal(t, int32(tasksCount), broker.TasksResubmitted)
}

func taskWithResubmitAndFailed(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	return NewFailAndResubmitTaskError(1*time.Minute, xerrors.New("some error"))
}

func Test_TaskWithResubmitAndFailed(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: tasksCount,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithResubmitAndFailed)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(1), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(tasksCount), broker.TasksFailed)
	assert.Equal(t, int32(0), broker.TasksDone)
	assert.Equal(t, int32(tasksCount), broker.TasksResubmitted)
}

func taskWithResubmit(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	return NewDoneAndResubmitTaskError(1 * time.Minute)
}

func Test_TaskWithPanic(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: 1,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, nil)
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithPanic)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(1), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(1), broker.TasksFailed)
	assert.Equal(t, int32(0), broker.TasksResubmitted)
}

func Test_TaskWithPanicAndWrapperRecover(t *testing.T) {
	logger := zaplogger.NewNop()

	broker := &FakeBroker{
		TotalTasksNumber: 1,
	}
	queue := NewQueue(broker)

	taskName := "task"
	err := queue.RegisterTask(taskName, SimpleRetryPolicy{
		Count:             0,
		Delay:             nil,
		RecoverErrWrapper: RecoverWithFailAndResubmit(10 * time.Minute),
	})
	if err != nil {
		t.Fatal(err)
	}
	err = queue.RegisterHandler(taskName, taskWithPanic)
	if err != nil {
		t.Fatal(err)
	}

	w := queue.NewWorker(taskName, logger, WithProcessNum(1), WithChunkFetchPeriod(10*time.Millisecond))

	timer := time.NewTimer(100 * time.Millisecond)
	go func() {
		<-timer.C
		w.Stop()
	}()

	_ = w.Launch()
	assert.Equal(t, int32(1), broker.TasksFailed)
	assert.Equal(t, int32(1), broker.TasksResubmitted)
}

func taskWithPanic(ctx context.Context, groupKey string, payload Payload, extra ExtraInfo) error {
	panic("handler: something really bad happened")
}
