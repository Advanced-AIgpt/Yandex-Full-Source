package nlg

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestDelayedAction(t *testing.T) {
	loc, err := time.LoadLocation("Europe/Moscow")
	assert.NoError(t, err)

	inputs := []struct {
		Now            time.Time
		ScheduledTime  time.Time
		ExpectedAnswer string
	}{
		{
			Now:            time.Date(2022, 12, 31, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 23, 59, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю сегодня в 23 часа 59 минут",
		},
		{
			Now:            time.Date(2022, 12, 30, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 23, 59, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю завтра в 23 часа 59 минут",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 23, 59, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю послезавтра в 23 часа 59 минут",
		},
		{
			Now:            time.Date(2022, 12, 1, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 23, 59, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю 31 декабря в 23 часа 59 минут",
		},
		{
			Now:            time.Date(2022, 12, 1, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 1, 1, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю 31 декабря в 1 час 1 минуту",
		},
		{
			Now:            time.Date(2022, 12, 1, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 2, 2, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю 31 декабря в 2 часа 2 минуты",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 12, 59, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю послезавтра в 12 часов 59 минут",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 12, 11, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю послезавтра в 12 часов 11 минут",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 31, 12, 17, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю послезавтра в 12 часов 17 минут",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 30, 1, 17, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю завтра в 1 час 17 минут",
		},
		{
			Now:            time.Date(2022, 12, 29, 23, 59, 00, 00, loc),
			ScheduledTime:  time.Date(2022, 12, 30, 4, 17, 00, 00, loc),
			ExpectedAnswer: "Хорошо, запомнила: сделаю завтра в 4 часа 17 минут",
		},
	}

	for _, input := range inputs {
		nlg := DelayedAction(input.Now, input.ScheduledTime, loc)
		assert.Len(t, nlg, 1)
		assert.Equal(t, nlg[0].Text(), input.ExpectedAnswer)
	}
}
