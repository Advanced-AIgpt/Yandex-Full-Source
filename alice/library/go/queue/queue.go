package queue

import (
	"context"
	"reflect"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func NewQueue(broker Broker) *Queue {
	return &Queue{
		tasks:  make(map[string]taskInfo),
		broker: broker,
	}
}

type Queue struct {
	tasks  map[string]taskInfo
	broker Broker
}

type taskInfo struct {
	handler     interface{}
	payloadType reflect.Type
	retryPolicy RetryPolicy
	mergePolicy *MergePolicy
}

func (q *Queue) RegisterTask(name string, retryPolicy RetryPolicy) error {
	q.tasks[name] = taskInfo{
		retryPolicy: retryPolicy,
	}

	return nil
}

func (q *Queue) SetMergePolicy(name string, policy MergePolicy) error {
	info, ok := q.tasks[name]
	if !ok {
		return xerrors.Errorf("queue: task with name `%s` is not registered", name)
	}

	info.mergePolicy = &policy
	q.tasks[name] = info
	return nil
}

func (q *Queue) RegisterHandler(name string, handler interface{}) error {
	if err := validateTaskFunction(handler); err != nil {
		return err
	}

	info, ok := q.tasks[name]
	if !ok {
		return xerrors.Errorf("queue: task with name `%s` is not registered", name)
	}

	info.handler = handler
	info.payloadType = reflect.ValueOf(handler).Type().In(2)
	q.tasks[name] = info
	return nil
}

func (q *Queue) SubmitTask(ctx context.Context, groupKey, name, mergeKey string, payload interface{}, scheduleTime time.Time) (string, error) {
	if _, ok := q.tasks[name]; !ok {
		return "", xerrors.Errorf("queue: task with name `%s` is not registered", name)
	}

	t, err := NewTask(name, groupKey, payload, scheduleTime)
	if err != nil {
		return "", err
	}

	if mergeKey != "" {
		t.SetMergeKey(mergeKey)
	}

	if err := q.broker.SubmitTasks(ctx, []Task{t}, q.GetTaskMergePolicy(t)); err != nil {
		return "", err
	}
	return t.ID, nil
}

func (q *Queue) SubmitTasks(ctx context.Context, tasks []Task) error {
	tasksByMergePolicy := make(map[MergePolicy][]Task)
	for _, t := range tasks {
		if _, ok := q.tasks[t.Name]; !ok {
			return xerrors.Errorf("queue: task with name `%s` is not registered", t.Name)
		}
		mergePolicy := q.GetTaskMergePolicy(t)
		tasksByMergePolicy[mergePolicy] = append(tasksByMergePolicy[mergePolicy], t)
	}

	for policy, tasks := range tasksByMergePolicy {
		if err := q.broker.SubmitTasks(ctx, tasks, policy); err != nil {
			return err
		}
	}
	return nil
}

func (q *Queue) NewWorker(taskName string, logger log.Logger, options ...WorkerOption) *Worker {
	config := getDefaultConfig()
	for _, opt := range options {
		opt(&config)
	}
	return newWorker(taskName, config, logger, q, q.broker)
}

func (q *Queue) GetTaskDelay(task Task) time.Duration {
	info := q.tasks[task.Name]
	if info.retryPolicy == nil {
		return 0
	}
	attempt := info.retryPolicy.GetTotalCount() - task.RetryLeft
	return info.retryPolicy.GetDelay(attempt)
}

func (q *Queue) GetTaskMergePolicy(task Task) MergePolicy {
	info := q.tasks[task.Name]
	if info.mergePolicy == nil {
		return Fail
	}
	return *info.mergePolicy
}

func (q *Queue) GetTaskInitialRetryCount(taskName string) int {
	info := q.tasks[taskName]
	if info.retryPolicy == nil {
		return 0
	}
	return info.retryPolicy.GetTotalCount()
}

func (q *Queue) GetRecoverErrorWrapperByTask(taskName string) func(error) error {
	if info, ok := q.tasks[taskName]; ok {
		if info.retryPolicy != nil {
			return info.retryPolicy.GetRecoverErrWrapper()
		}
	}
	return nil
}

func (q *Queue) callHandler(ctx context.Context, task Task, overdue time.Duration) (handlerErr error) {
	defer func() {
		if r := recover(); r != nil {
			handlerErr = NewPanicOnTaskError(r) // wrap panic to the special error
			onRecoverWrapper := q.GetRecoverErrorWrapperByTask(task.Name)
			if onRecoverWrapper != nil {
				handlerErr = onRecoverWrapper(handlerErr)
			}
		}
	}()

	regTask := q.tasks[task.Name]
	taskFunc := reflect.ValueOf(regTask.handler)
	taskContext := reflect.ValueOf(ctx)
	taskGroupKey := reflect.ValueOf(task.GroupKey)
	taskExtra := reflect.ValueOf(ExtraInfo{
		RetryLeft: task.RetryLeft,
		Overdue:   overdue,
	})
	payload, err := decodeJSONToInterface(task.Payload, regTask.payloadType)
	if err != nil {
		return err
	}
	taskPayload := reflect.ValueOf(payload)

	ret := taskFunc.Call([]reflect.Value{taskContext, taskGroupKey, taskPayload, taskExtra})

	taskErr := ret[0].Interface()
	if taskErr != nil {
		handlerErr = taskErr.(error)
	}
	return
}

func validateTaskFunction(taskFunc interface{}) error {
	v := reflect.ValueOf(taskFunc)
	t := v.Type()

	if t.Kind() != reflect.Func {
		return xerrors.Errorf("Passed parameter is not a function but %s", t.Name())
	}

	if t.NumIn() != 4 {
		return xerrors.Errorf("Function must have exactly 4 parameters but has %d", t.NumIn())
	}

	contextIface := reflect.TypeOf((*context.Context)(nil)).Elem()
	if !t.In(0).Implements(contextIface) {
		return xerrors.Errorf("The first param must implement context.Context interface")
	}

	if t.In(1).Kind() != reflect.String {
		return xerrors.Errorf("The second param should be string groupKey")
	}

	if t.In(2).Kind() != reflect.Struct {
		return xerrors.Errorf("The third param should be struct which is serializable with json.Marshal")
	}

	if t.In(3).Kind() != reflect.Struct || t.In(3) != reflect.TypeOf(ExtraInfo{}) {
		return xerrors.Errorf("The fourth param should be struct of type ExtraInfo")
	}

	if t.NumOut() != 1 {
		return xerrors.Errorf("Function should return only one value")
	}

	returnType := t.Out(0)
	errorIface := reflect.TypeOf((*error)(nil)).Elem()
	if !returnType.Implements(errorIface) {
		return xerrors.Errorf("Return parameter doesn't implement error interface")
	}

	return nil
}
