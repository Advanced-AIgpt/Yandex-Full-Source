package queue

import (
	"time"
)

type WorkerOption func(*WorkerConfig)

func WithProcessNum(num int) WorkerOption {
	return func(config *WorkerConfig) {
		config.ProcessNum = num
	}
}

func WithChunkSize(num int) WorkerOption {
	return func(config *WorkerConfig) {
		config.TasksChunkSize = num
	}
}

func WithChunkFetchPeriod(duration time.Duration) WorkerOption {
	return func(config *WorkerConfig) {
		config.TasksFetchPeriod = duration
	}
}

func WithSignalsRegistry(registry *SignalsRegistry) WorkerOption {
	return func(config *WorkerConfig) {
		config.Registry = registry
	}
}

type WorkerConfig struct {
	ProcessNum                 int
	TasksChunkSize             int
	TasksFetchPeriod           time.Duration
	HeartbeatPeriod            time.Duration
	ShutdownWaitPeriod         time.Duration
	StatisticsCollectionPeriod time.Duration
	LostTasksFetchPeriod       time.Duration
	LostTasksFoundFetchPeriod  time.Duration
	DatabaseExecutionTimeout   time.Duration

	OldTasksCleanupPeriod    time.Duration
	OldTasksCleanupThreshold time.Duration
	OldTasksCleanupLimit     int

	Registry *SignalsRegistry
}

func getDefaultConfig() WorkerConfig {
	return WorkerConfig{
		ProcessNum:                 10,
		TasksChunkSize:             100,
		TasksFetchPeriod:           500 * time.Millisecond,
		HeartbeatPeriod:            1 * time.Minute,
		ShutdownWaitPeriod:         10 * time.Second,
		StatisticsCollectionPeriod: 1 * time.Minute,
		LostTasksFetchPeriod:       1 * time.Minute,
		LostTasksFoundFetchPeriod:  5 * time.Second,
		DatabaseExecutionTimeout:   10 * time.Second,
		OldTasksCleanupPeriod:      5 * time.Minute,
		OldTasksCleanupLimit:       10000,
		OldTasksCleanupThreshold:   7 * 24 * time.Hour,
	}
}
