package megamind

import (
	"encoding/json"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestDatetimeConversion(t *testing.T) {
	inputs := []struct {
		Name                    string
		DateTimeJSON            string
		Now                     time.Time
		ExpectedTime            time.Time
		ExpectedIsTimeSpecified bool
	}{
		{
			Name:                    "через полтора часа",
			DateTimeJSON:            `{"hours":1,"hours_relative":true,"minutes":30,"minutes_relative":true}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 17, 18, 31, 25, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "через 1 день и 5 секунд",
			DateTimeJSON:            `{"days":1,"days_relative":true,"seconds":5,"seconds_relative":true}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 18, 17, 01, 30, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "в 17:42 (сейчас 15:30) - сегодня, через 2 часа 12 минут",
			DateTimeJSON:            `{"hours":17,"minutes":42}`,
			Now:                     time.Date(2020, 7, 17, 15, 30, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 17, 17, 42, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "в 17:42 (сейчас 18:30) - завтра",
			DateTimeJSON:            `{"hours":17,"minutes":42}`,
			Now:                     time.Date(2020, 7, 17, 18, 30, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 18, 17, 42, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "через 1 месяц, 2 дня и 17 минут",
			DateTimeJSON:            `{"days":2,"days_relative":true,"minutes":17,"minutes_relative":true,"months":1,"months_relative":true}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 19, 17, 18, 25, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "через 11 месяцев и 20 дней",
			DateTimeJSON:            `{"days":20,"days_relative":true,"months":11,"months_relative":true}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2021, 7, 7, 17, 01, 25, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "через 11 месяцев",
			DateTimeJSON:            `{"months":11,"months_relative":true}`,
			Now:                     time.Date(2020, 3, 30, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2021, 3, 2, 17, 01, 25, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "через 1 год, 11 месяцев и 20 дней",
			DateTimeJSON:            `{"date_relative":true,"days":20,"months":11,"years":1}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2022, 7, 7, 17, 01, 25, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "5 ноября 2100 года",
			DateTimeJSON:            `{"days":5,"months":11,"years":2100}`,
			Now:                     time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2100, 11, 5, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "7 ноября 1988",
			DateTimeJSON:            `{"days":7,"months":11,"years":1988}`,
			Now:                     time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:            time.Date(1988, 11, 7, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "1 января 2000 года в 00:15",
			DateTimeJSON:            `{"days":1,"hours":0,"minutes":15,"months":1,"years":2000}`,
			Now:                     time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2000, 1, 1, 0, 15, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "завтра в 7.00",
			DateTimeJSON:            `{"days":1,"days_relative":true,"hours":7,"minutes":0}`,
			Now:                     time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 18, 7, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "10 января (текущая дата до 10 января) - без года подразумеваем, что это 10 января текущего года",
			DateTimeJSON:            `{"days":10,"months":1}`,
			Now:                     time.Date(2020, 1, 7, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 1, 10, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "10 января (текущая дата 17 января) - без года подразумеваем, что это 10 января в текущем году (прошлое), т.к. 17 января следующего года дальше, чем 90 дней",
			DateTimeJSON:            `{"days":10,"months":1}`,
			Now:                     time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 1, 10, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "10 января в 15:00 (сейчас 10 января 16:00) - предполагаем, что это прошлое, т.к. если бы это было будущее, то оно дальше, чем 90 дней",
			DateTimeJSON:            `{"days":10,"months":1,"hours":15,"minutes":0}`,
			Now:                     time.Date(2020, 1, 10, 16, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 1, 10, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "20 числа (текущая дата в месяце меньше 20-ого) - без месяца подразумеваем, что это текущий месяц",
			DateTimeJSON:            `{"days":20}`,
			Now:                     time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 1, 20, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "20 числа (текущая дата в месяце больше 20-ого) - без месяца подразумеваем, что это 20 число следующего месяца",
			DateTimeJSON:            `{"days":20}`,
			Now:                     time.Date(2020, 1, 27, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 2, 20, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "В четверг в 15.00 (сейчас вторник) - подразумеваем, что это четверг на этой неделе",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":4}`,
			Now:                     time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 13, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В воскресенье в 15.00 (сейчас вторник) - подразумеваем, что это воскресенье на этой неделе",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":7}`,
			Now:                     time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 16, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":1}`,
			Now:                     time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В следующий понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":1,"weeks":1,"weeks_relative":true}`,
			Now:                     time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В следующую среду в 17:00 (сейчас вторник) - подразумеваем, что это среда на следующей неделе",
			DateTimeJSON:            `{"hours":17,"minutes":0,"weekday":3,"weeks":1,"weeks_relative":true}`,
			Now:                     time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 19, 17, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В понедельник в 15.00 (сейчас понедельник 10.00) - подразумеваем, что это сегодня",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":1}`,
			Now:                     time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В понедельник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это следующий понедельник",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":1}`,
			Now:                     time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Во вторник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это завтра",
			DateTimeJSON:            `{"hours":15,"minutes":0,"weekday":2}`,
			Now:                     time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 10 часов (сейчас уже 11) - подразумеваем, что это про 10 вечера сегодня",
			DateTimeJSON:            `{"hours":10,"minutes":0}`,
			Now:                     time.Date(2020, 7, 17, 11, 1, 25, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 7, 17, 22, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 10 числа в 15.00 (сейчас 10-е число 10.00) - подразумеваем, что это сегодня",
			DateTimeJSON:            `{"days":10,"hours":15,"minutes":0}`,
			Now:                     time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 10 числа в 15.00 (сейчас 10-е число 18.00) - подразумеваем, что это следующий месяц",
			DateTimeJSON:            `{"days":10,"hours":15,"minutes":0}`,
			Now:                     time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 9, 10, 15, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сейчас - убираем время совсем, в time.IsZero() == true, как будто и не указывали",
			DateTimeJSON:            `{"seconds":0,"seconds_relative":true}`,
			Now:                     time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Time{},
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня - должна быть сегодняшняя дата и IsTimeSpecified - false",
			DateTimeJSON:            `{"days":0,"days_relative":true}`,
			Now:                     time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "Во вторник (сегодня вторник) - должна быть сегодняшняя дата и IsTimeSpecified - false",
			DateTimeJSON:            `{"weekday":2}`,
			Now:                     time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "В среду (сегодня вторник) - должна быть завтрашняя дата и IsTimeSpecified - false",
			DateTimeJSON:            `{"weekday":3}`,
			Now:                     time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 0, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "Через неделю - должна быть дата через неделю и IsTimeSpecified - false",
			DateTimeJSON:            `{"weeks":1,"weeks_relative":true}`,
			Now:                     time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 18, 18, 15, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: false,
		},
		{
			Name:                    "Сегодня в два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			DateTimeJSON:            `{"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня в два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня в семь часов утра (сейчас 11 вечера) - имеется ввиду завтра, будущее",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":7,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 7, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В семь часов утра (сейчас 11 вечера) - имеется ввиду сегодня, будущее",
			DateTimeJSON:            `{"hours":7,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 7, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня в семь часов утра (сейчас 5 утра) - имеется ввиду сегодня, будущее",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":7,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В семь часов утра (сейчас 11 вечера) - имеется ввиду сегодня, будущее",
			DateTimeJSON:            `{"hours":7,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Завтра в два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			DateTimeJSON:            `{"days":1,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Завтра в два часа ночи (сейчас 1 ночи) - должно быть ночью на след день",
			DateTimeJSON:            `{"days":1,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Вчера в два часа ночи - должно быть прошлое",
			DateTimeJSON:            `{"days":-1,"days_relative":true,"hours":2,"minutes":0,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 10, 2, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 5 дня",
			DateTimeJSON:            `{"hours":5,"period":"pm"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 5 часов (сейчас время до 5 утра, поэтому думаем, что это 5 утра)",
			DateTimeJSON:            `{"hours":5}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 5 часов (сейчас время после 5 утра, поэтому думаем, что это 5 вечера)",
			DateTimeJSON:            `{"hours":5}`,
			Now:                     time.Date(2020, 8, 11, 6, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 17 часов (сейчас время до 5 утра, но мы явно указали вечернее время)",
			DateTimeJSON:            `{"hours":17}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "В 5 часов утра",
			DateTimeJSON:            `{"hours":5,"period":"am"}`,
			Now:                     time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня в 1 час (сейчас 14:42) - завтра в час ночи",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":1,"minutes":0}`,
			Now:                     time.Date(2020, 8, 11, 14, 42, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 1, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
		{
			Name:                    "Сегодня в 13 часов (сейчас 13:42) - завтра в час дня",
			DateTimeJSON:            `{"days":0,"days_relative":true,"hours":13}`,
			Now:                     time.Date(2020, 8, 11, 13, 42, 0, 0, time.UTC),
			ExpectedTime:            time.Date(2020, 8, 12, 13, 0, 0, 0, time.UTC),
			ExpectedIsTimeSpecified: true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var d DateTime
			err := json.Unmarshal([]byte(input.DateTimeJSON), &d)
			assert.NoError(t, err)

			assert.False(t, d.IsInterval())
			dateTime := d.ToDateTime(input.Now)

			assert.InDelta(t, 0, dateTime.Sub(input.ExpectedTime), float64(time.Second))
			assert.Equal(t, input.ExpectedIsTimeSpecified, dateTime.IsTimeSpecified)
		})
	}
}

func TestIntervalsConversion(t *testing.T) {
	inputs := []struct {
		Name                         string
		DateTimeJSON                 string
		Now                          time.Time
		ExpectedStartTime            time.Time
		ExpectedEndTime              time.Time
		ExpectedIsStartTimeSpecified bool
		ExpectedIsEndTimeSpecified   bool
	}{
		{
			Name:                         "на 10 минут",
			DateTimeJSON:                 `{"end":{"minutes":10,"minutes_relative":true},"start":{"minutes":0,"minutes_relative":true}}`,
			Now:                          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedStartTime:            time.Time{},
			ExpectedIsStartTimeSpecified: true,
			ExpectedEndTime:              time.Date(2020, 7, 17, 17, 11, 25, 0, time.UTC),
			ExpectedIsEndTimeSpecified:   true,
		},
		{
			Name:                         "на 10 минут через полчаса",
			DateTimeJSON:                 `{"end":{"minutes":10,"minutes_relative":true},"start":{"minutes":30,"minutes_relative":true}}`,
			Now:                          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedStartTime:            time.Date(2020, 7, 17, 17, 31, 25, 0, time.UTC),
			ExpectedIsStartTimeSpecified: true,
			ExpectedEndTime:              time.Date(2020, 7, 17, 17, 41, 25, 0, time.UTC),
			ExpectedIsEndTimeSpecified:   true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var d DateTime
			err := json.Unmarshal([]byte(input.DateTimeJSON), &d)
			assert.NoError(t, err)

			assert.True(t, d.IsInterval())
			startDateTime, endDateTime := d.ToInterval(input.Now)

			assert.InDelta(t, 0, startDateTime.Sub(input.ExpectedStartTime), float64(time.Second))
			assert.InDelta(t, 0, endDateTime.Sub(input.ExpectedEndTime), float64(time.Second))
			assert.Equal(t, input.ExpectedIsStartTimeSpecified, startDateTime.IsTimeSpecified)
			assert.Equal(t, input.ExpectedIsEndTimeSpecified, endDateTime.IsTimeSpecified)
		})
	}
}

func TestUnmarshalHypotheses(t *testing.T) {
	data := `[
   {
      "action":{
         "instance":"on",
         "type":"devices.capabilities.on_off",
         "value":true
      },
      "devices":[
         "demo--other8"
      ],
      "id":0,
      "nlg":{
         "variants":[
            "Ð\\x92ÐºÐ»Ñ\\x8eÑ\\x87Ð°Ñ\\x8e."
         ]
      },
      "raw_entities":[
         {
            "end":1,
            "extra":null,
            "start":0,
            "text":"Ð²ÐºÐ»Ñ\\x8eÑ\\x87Ð¸",
            "type":"bow_action",
            "value":"Ð²ÐºÐ»Ñ\\x8eÑ\\x87Ð°Ñ\\x82Ñ\\x8c"
         },
         {
            "end":2,
            "extra":{
               "ids":[
                  "demo--other8"
               ]
            },
            "start":1,
            "text":"Ð¾Ð±Ð¾Ð³Ñ\\x80ÐµÐ²Ð°Ñ\\x82ÐµÐ»Ñ\\x8c",
            "type":"device",
            "value":[
               "device--demo--other8"
            ]
         },
         {
            "end":3,
            "extra":null,
            "start":2,
            "text":"Ð²",
            "type":"preposition",
            "value":null
         }
      ],
      "request_type":"action",
      "rooms":[
         "demo--bathroom"
      ]
   }
]`

	var hs Hypotheses
	err := json.Unmarshal([]byte(data), &hs)
	assert.NoError(t, err)

	hypothesis := hs[0]
	typeToDemoEntities := make(map[string][]RawEntity)
	for _, e := range hypothesis.RawEntities {
		if e.IsDemoEntity() {
			typeToDemoEntities[e.Type] = append(typeToDemoEntities[e.Type], e)
		}
	}

	assert.Equal(t, 1, len(typeToDemoEntities))
}
