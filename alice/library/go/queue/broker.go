package queue

import (
	"context"
	"time"
)

type Broker interface {
	SubmitTasks(ctx context.Context, tasks []Task, mergePolicy MergePolicy) error
	GetTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]Task, error)
	GetLostTasksAndUpdateState(ctx context.Context, taskName string, limit int) ([]Task, error)
	SetTasksReady(ctx context.Context, tasks []Task) error
	SetTasksDone(ctx context.Context, tasks []Task) error
	SetTasksFinishedAndSubmit(ctx context.Context, finishedTasks []Task, newTasks []Task, mergePolicy MergePolicy) error
	SetTasksFailed(ctx context.Context, tasks []Task) error
	RemoveOldFinishedTasks(ctx context.Context, taskName string, threshold time.Duration, limit int) (int, error)

	Heartbeat(ctx context.Context)
	Cleanup(ctx context.Context)
}
