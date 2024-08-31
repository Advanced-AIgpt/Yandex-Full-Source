package queue

import (
	"fmt"
	"time"
)

type DoneAndResubmitTaskError struct {
	delay time.Duration
}

func (r DoneAndResubmitTaskError) Error() string {
	return fmt.Sprintf("queue: resubmit task in %.2f seconds", r.delay.Seconds())
}

func NewDoneAndResubmitTaskError(delay time.Duration) error {
	return DoneAndResubmitTaskError{delay: delay}
}

type FailAndResubmitTaskError struct {
	delay time.Duration
	err   error
}

func (r FailAndResubmitTaskError) Error() string {
	return fmt.Sprintf("queue: resubmit task in %.2f seconds", r.delay.Seconds())
}

func NewFailAndResubmitTaskError(delay time.Duration, err error) error {
	return FailAndResubmitTaskError{delay: delay, err: err}
}

type PanicOnTaskError struct {
	recoverResult any
}

func (p PanicOnTaskError) Error() string {
	return fmt.Sprintf("task processing caught panic: %+v", p.recoverResult)
}

func NewPanicOnTaskError(recoveryResult any) error {
	return PanicOnTaskError{recoverResult: recoveryResult}
}
