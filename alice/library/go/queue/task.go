package queue

import (
	"encoding/json"
	"time"

	"github.com/gofrs/uuid"
)

type State string

const (
	Ready   State = "READY"
	Running State = "RUNNING"
	Done    State = "DONE"
	Failed  State = "FAILED"
)

type MergePolicy int

const (
	Fail MergePolicy = iota
	Ignore
	Replace
)

type Task struct {
	ID            string        // task identifier, uuid
	GroupKey      string        // key for grouping task, e.g. user id
	Name          string        // task name, needed to specify handler function for the task
	State         State         // task state
	Payload       []byte        // raw json payload
	CreatedTime   time.Time     // time when task was created, used for debug purposes
	ScheduledTime time.Time     // time when task will be fetched from broker and passed to handler
	UpdatedTime   time.Time     // field for long-running tasks; should be updated by handler if processing takes longer than 5 minutes
	Timeout       time.Duration // task max duration required for processing; handler's first param (context.Context) will be cancelled when time is over; default is 0 - no timeout
	RetryLeft     int           // retries left for the current task; value 0 means no retry
	LastError     *string       // last error occurred for tasks with retries left and for failed tasks
	MergeKey      *string       // key for tasks deduplication (when you need only one task in queue for some reason)
}

func NewTask(name, groupKey string, payload interface{}, scheduledTime time.Time) (Task, error) {
	return NewTaskWithTimeout(name, groupKey, payload, scheduledTime, 0)
}

func NewTaskWithTimeout(name, groupKey string, payload interface{}, scheduledTime time.Time, timeout time.Duration) (Task, error) {
	encodedPayload, err := json.Marshal(payload)
	if err != nil {
		return Task{}, err
	}

	return Task{
		ID:            GenerateTaskID(),
		GroupKey:      groupKey,
		Name:          name,
		State:         Ready,
		Payload:       encodedPayload,
		CreatedTime:   time.Now().UTC(),
		ScheduledTime: scheduledTime.UTC(),
		Timeout:       timeout,
	}, nil
}

func (t *Task) SetMergeKey(key string) {
	t.MergeKey = &key
}

func GenerateTaskID() string {
	return uuid.Must(uuid.NewV4()).String()
}

type ExtraInfo struct {
	RetryLeft int
	Overdue   time.Duration
}
