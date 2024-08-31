package common

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/ptr"
)

func TestParseBegemotDateAndTimeV2(t *testing.T) {
	moscowTimezone, _ := time.LoadLocation("Europe/Moscow")
	newYorkTimezone, _ := time.LoadLocation("America/New_York")

	inputs := []struct {
		Name                  string
		ExactDateJSON         string
		ExactTimeJSON         string
		RelativeDateTimeJSONs []string
		Now                   time.Time
		ExpectedTime          time.Time
	}{
		{
			Name: "через полтора часа",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"hours":1,"hours_relative":true,"minutes":30,"minutes_relative":true},
					"start":{"hours":0,"hours_relative":true,"minutes":0,"minutes_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 17, 18, 31, 25, 0, time.UTC),
		},
		{
			Name:          "в пять – в канун нового года",
			ExactTimeJSON: `{"hours":5}`,
			Now:           time.Date(2020, 12, 31, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2021, 1, 1, 5, 0, 0, 0, time.UTC),
		},
		{
			Name: "через 1 день и 5 секунд",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"days":1,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
				`{
					"end":{"seconds":5,"seconds_relative":true},
					"start":{"seconds":0,"seconds_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 18, 17, 01, 30, 0, time.UTC),
		},
		{
			Name:          "в 17:42 (сейчас 15:30) - сегодня, через 2 часа 12 минут",
			ExactTimeJSON: `{"hours":17,"minutes":42}`,
			Now:           time.Date(2020, 7, 17, 15, 30, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 17, 17, 42, 0, 0, time.UTC),
		},
		{
			Name:          "в 17:42 (сейчас 18:30) - завтра",
			ExactTimeJSON: `{"hours":17,"minutes":42}`,
			Now:           time.Date(2020, 7, 17, 18, 30, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 18, 17, 42, 0, 0, time.UTC),
		},
		{
			Name: "через 1 месяц, 2 дня и 17 минут",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"months":1,"months_relative":true},
					"start":{"months":0,"months_relative":true}
				}`,
				`{
					"end":{"days":2,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
				`{
					"end":{"minutes":17,"minutes_relative":true},
					"start":{"minutes":0,"minutes_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 19, 17, 18, 25, 0, time.UTC),
		},
		{
			Name: "через 11 месяцев и 20 дней",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"months":11,"months_relative":true},
					"start":{"months":0,"months_relative":true}
				}`,
				`{
					"end":{"days":20,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2021, 7, 7, 17, 01, 25, 0, time.UTC),
		},
		{
			Name: "через 11 месяцев",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"months":11,"months_relative":true},
					"start":{"months":0,"months_relative":true}
				}`,
			},
			Now:          time.Date(2020, 3, 30, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2021, 3, 2, 17, 01, 25, 0, time.UTC),
		},
		{
			Name: "через 1 год, 11 месяцев и 20 дней",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"years":1,"years_relative":true},
					"start":{"years":0,"years_relative":true}
				}`,
				`{
					"end":{"months":11,"months_relative":true},
					"start":{"months":0,"months_relative":true}
				}`,
				`{
					"end":{"days":20,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2022, 7, 7, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:          "5 ноября 2100 года",
			ExactDateJSON: `{"days":5,"months":11,"years":2100}`,
			Now:           time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2100, 11, 5, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:          "7 ноября 1988",
			ExactDateJSON: `{"days":7,"months":11,"years":1988}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(1988, 11, 7, 17, 1, 25, 0, time.UTC),
		},
		{
			Name:          "1 января 2000 года в 00:15",
			ExactDateJSON: `{"days":1,"months":1,"years":2000}`,
			ExactTimeJSON: `{"hours":0,"minutes":15}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2000, 1, 1, 0, 15, 0, 0, time.UTC),
		},
		{
			Name:          "завтра в 7.00",
			ExactDateJSON: `{"days":1,"days_relative":true}`,
			ExactTimeJSON: `{"hours":7,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 18, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:          "завтра в 7.00 с таймзоной",
			ExactDateJSON: `{"days":1,"days_relative":true}`,
			ExactTimeJSON: `{"hours":7,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, moscowTimezone),
			ExpectedTime:  time.Date(2020, 7, 18, 7, 0, 0, 0, moscowTimezone),
		},
		{
			Name:          "завтра в 7.00 с другой таймзоной",
			ExactDateJSON: `{"days":1,"days_relative":true}`,
			ExactTimeJSON: `{"hours":7,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, newYorkTimezone),
			ExpectedTime:  time.Date(2020, 7, 18, 7, 0, 0, 0, newYorkTimezone),
		},
		{
			Name: "через день в 7.00",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"days":1,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
			},
			ExactTimeJSON: `{"hours":7,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 18, 7, 0, 0, 0, time.UTC),
		},
		{
			Name: "через неделю в четыре",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"weeks":1,"weeks_relative":true},
					"start":{"weeks":0,"weeks_relative":true}
				}`,
			},
			ExactTimeJSON: `{"hours":4,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 24, 4, 0, 0, 0, time.UTC),
		},
		{
			Name: "через неделю в пять вечера",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"weeks":1,"weeks_relative":true},
					"start":{"weeks":0,"weeks_relative":true}
				}`,
			},
			ExactTimeJSON: `{"hours":5,"minutes":0,"period":"pm"}`,
			Now:           time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 24, 17, 0, 0, 0, time.UTC),
		},
		{
			Name: "через полтора дня",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"days":1,"days_relative":true,"hours":12,"hours_relative":true},
					"start":{"days":0,"days_relative":true,"hours":0,"hours_relative":true}
				}`,
			},
			Now:          time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 19, 5, 1, 25, 0, time.UTC),
		},
		{
			Name:          "10 января (текущая дата до 10 января) - без года подразумеваем, что это 10 января текущего года",
			ExactDateJSON: `{"days":10,"months":1}`,
			Now:           time.Date(2020, 1, 7, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 1, 10, 10, 12, 13, 0, time.UTC),
		},
		{
			Name:          "10 января (текущая дата 17 января) - без года подразумеваем, что это 10 января в следующем году",
			ExactDateJSON: `{"days":10,"months":1}`,
			Now:           time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2021, 1, 10, 10, 12, 13, 0, time.UTC),
		},
		{
			Name:          "10 января в 15:00 (сейчас 10 января 16:00) - предполагаем, что это в следующем году",
			ExactDateJSON: `{"days":10,"months":1}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 1, 10, 16, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2021, 1, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "20 числа (текущая дата в месяце меньше 20-ого) - без месяца подразумеваем, что это текущий месяц",
			ExactDateJSON: `{"days":20}`,
			Now:           time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 1, 20, 10, 12, 13, 0, time.UTC),
		},
		{
			Name:          "20 числа (текущая дата в месяце больше 20-ого) - без месяца подразумеваем, что это 20 число следующего месяца",
			ExactDateJSON: `{"days":20}`,
			Now:           time.Date(2020, 1, 27, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 2, 20, 10, 12, 13, 0, time.UTC),
		},
		{
			Name:          "В четверг в 15.00 (сейчас вторник) - подразумеваем, что это четверг на этой неделе",
			ExactDateJSON: `{"weekday":4}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 13, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В воскресенье в 15.00 (сейчас вторник) - подразумеваем, что это воскресенье на этой неделе",
			ExactDateJSON: `{"weekday":7}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 16, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			ExactDateJSON: `{"weekday":1}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В следующий понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			ExactDateJSON: `{"weekday":1,"weeks":1,"weeks_relative":true}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В следующую среду в 17:00 (сейчас вторник) - подразумеваем, что это среда на этой неделе",
			ExactDateJSON: `{"weekday":3,"weeks":1,"weeks_relative":true}`,
			ExactTimeJSON: `{"hours":17,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В понедельник в 15.00 (сейчас понедельник 10.00) - подразумеваем, что это сегодня",
			ExactDateJSON: `{"weekday":1}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В понедельник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это следующий понедельник",
			ExactDateJSON: `{"weekday":1}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Во вторник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это завтра",
			ExactDateJSON: `{"weekday":2}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В четверг 8 сентября 2021 – такого дня нет. Ищем ближайший четверг к этому числу",
			ExactDateJSON: `{"weekday":4, "days": 8, "months": 9, "years": 2021}`,
			Now:           time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2021, 9, 9, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:          "В 10 часов (сейчас уже 11) - подразумеваем, что это про 10 вечера сегодня",
			ExactTimeJSON: `{"hours":10,"minutes":0}`,
			Now:           time.Date(2020, 7, 17, 11, 1, 25, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 7, 17, 22, 0, 0, 0, time.UTC),
		},
		{
			Name:          "10 числа в 15.00 (сейчас 10-е число 10.00) - подразумеваем, что это сегодня",
			ExactDateJSON: `{"days":10}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "10 числа в 15.00 (сейчас 10-е число 18.00) - подразумеваем, что это следующий месяц",
			ExactDateJSON: `{"days":10}`,
			ExactTimeJSON: `{"hours":15,"minutes":0}`,
			Now:           time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 9, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сейчас - возвращаем текущее время",
			ExactDateJSON: `{"seconds":0,"seconds_relative":true}`,
			Now:           time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня - должна быть сегодняшняя дата",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			Now:           time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:          "Во вторник (сегодня вторник) - должна быть сегодняшняя дата",
			ExactDateJSON: `{"weekday":2}`,
			Now:           time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:          "В среду (сегодня вторник) - должна быть завтрашняя дата",
			ExactDateJSON: `{"weekday":3}`,
			Now:           time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 18, 15, 0, 0, time.UTC),
		},
		{
			Name: "Через неделю - должна быть дата через неделю",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"weeks":1,"weeks_relative":true},
					"start":{"weeks":0,"weeks_relative":true}
				}`,
			},
			Now:          time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 18, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в два часа ночи (сейчас 11 вечера) - должно быть в прошлом",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в семь часов утра (сейчас 11 вечера) - прошлое",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":7,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В семь часов утра (сейчас 11 вечера) - имеется ввиду завтра, будущее",
			ExactTimeJSON: `{"hours":7,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в семь часов утра (сейчас 5 утра) - имеется ввиду сегодня, будущее",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":7,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В семь часов утра (сейчас 5 утра) - имеется ввиду сегодня, будущее",
			ExactTimeJSON: `{"hours":7,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Завтра в два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			ExactDateJSON: `{"days":1,"days_relative":true}`,
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name: "Завтра в два часа ночи (сейчас 1 ночи) - должно быть ночью на след день",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"days":1,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
			},
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name: "Вчера в два часа ночи - должно быть прошлое",
			RelativeDateTimeJSONs: []string{
				`{
					"end":{"days":-1,"days_relative":true},
					"start":{"days":0,"days_relative":true}
				}`,
			},
			ExactTimeJSON: `{"hours":2,"minutes":0,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 10, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В 5 дня",
			ExactTimeJSON: `{"hours":5,"period":"pm"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В 5 часов (сейчас время до 5 утра, поэтому думаем, что это 5 утра)",
			ExactTimeJSON: `{"hours":5}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В 5 часов (сейчас время после 5 утра, поэтому думаем, что это 5 вечера)",
			ExactTimeJSON: `{"hours":5}`,
			Now:           time.Date(2020, 8, 11, 6, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В 17 часов (сейчас время до 5 утра, но мы явно указали вечернее время)",
			ExactTimeJSON: `{"hours":17}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:          "В 5 часов утра",
			ExactTimeJSON: `{"hours":5,"period":"am"}`,
			Now:           time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в 1 час (сейчас 14:42) - прошлое",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":1,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 14, 42, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
		},
		{
			Name:          "Сегодня в 13 часов (сейчас 13:42) - завтра в час дня",
			ExactDateJSON: `{"days":0,"days_relative":true}`,
			ExactTimeJSON: `{"hours":13,"minutes":0}`,
			Now:           time.Date(2020, 8, 11, 13, 42, 0, 0, time.UTC),
			ExpectedTime:  time.Date(2020, 8, 11, 13, 0, 0, 0, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var exactDate BegemotDate
			var exactTime BegemotTime
			var relativeDateTimes []*BegemotDateTimeRange

			if input.ExactDateJSON != "" {
				err := exactDate.FromValueString(input.ExactDateJSON)
				assert.NoError(t, err)
			}

			if input.ExactTimeJSON != "" {
				err := exactTime.FromValueString(input.ExactTimeJSON)
				assert.NoError(t, err)
			}

			if len(input.RelativeDateTimeJSONs) > 0 {
				for _, relativeDateTimeJSON := range input.RelativeDateTimeJSONs {
					var dateTime BegemotDateTimeRange
					err := dateTime.FromValueString(relativeDateTimeJSON)
					assert.NoError(t, err)
					relativeDateTimes = append(relativeDateTimes, &dateTime)
				}
			}

			dateTime := ParseBegemotDateAndTimeV2(input.Now, &exactDate, &exactTime, relativeDateTimes)
			assert.Equal(t, input.ExpectedTime, dateTime)
			assert.InDelta(t, 0, dateTime.Sub(input.ExpectedTime), float64(time.Second))
		})
	}
}

func TestDatetimeConversion(t *testing.T) {
	inputs := []struct {
		Name         string
		DateJSON     string
		TimeJSON     string
		Now          time.Time
		ExpectedTime time.Time
	}{
		{
			Name:         "через полтора часа",
			TimeJSON:     `{"hours":1,"hours_relative":true,"minutes":30,"minutes_relative":true}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 17, 18, 31, 25, 0, time.UTC),
		},
		{
			Name:         "в пять – в канун нового года",
			TimeJSON:     `{"hours":5}`,
			Now:          time.Date(2020, 12, 31, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2021, 1, 1, 5, 0, 0, 0, time.UTC),
		},
		{
			Name:         "через 1 день и 5 секунд",
			DateJSON:     `{"days":1,"days_relative":true}`,
			TimeJSON:     `{"seconds":5,"seconds_relative":true}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 18, 17, 01, 30, 0, time.UTC),
		},
		{
			Name:         "в 17:42 (сейчас 15:30) - сегодня, через 2 часа 12 минут",
			TimeJSON:     `{"hours":17,"minutes":42}`,
			Now:          time.Date(2020, 7, 17, 15, 30, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 17, 17, 42, 0, 0, time.UTC),
		},
		{
			Name:         "в 17:42 (сейчас 18:30) - завтра",
			TimeJSON:     `{"hours":17,"minutes":42}`,
			Now:          time.Date(2020, 7, 17, 18, 30, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 18, 17, 42, 0, 0, time.UTC),
		},
		{
			Name:         "через 1 месяц, 2 дня и 17 минут",
			DateJSON:     `{"days":2,"days_relative":true,"months":1,"months_relative":true}`,
			TimeJSON:     `{"minutes":17,"minutes_relative":true}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 19, 17, 18, 25, 0, time.UTC),
		},
		{
			Name:         "через 11 месяцев и 20 дней",
			DateJSON:     `{"days":20,"days_relative":true,"months":11,"months_relative":true}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2021, 7, 7, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:         "через 11 месяцев",
			DateJSON:     `{"months":11,"months_relative":true}`,
			Now:          time.Date(2020, 3, 30, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2021, 3, 2, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:         "через 1 год, 11 месяцев и 20 дней",
			DateJSON:     `{"date_relative":true,"days":20,"months":11,"years":1}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2022, 7, 7, 17, 01, 25, 0, time.UTC),
		},
		{
			Name:         "5 ноября 2100 года",
			DateJSON:     `{"days":5,"months":11,"years":2100}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2100, 11, 5, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "7 ноября 1988",
			DateJSON:     `{"days":7,"months":11,"years":1988}`,
			Now:          time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime: time.Date(1988, 11, 7, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "1 января 2000 года в 00:15",
			DateJSON:     `{"days":1,"hours":0,"minutes":15,"months":1,"years":2000}`,
			TimeJSON:     `{"hours":0,"minutes":15}`,
			Now:          time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime: time.Date(2000, 1, 1, 0, 15, 0, 0, time.UTC),
		},
		{
			Name:         "завтра в 7.00",
			DateJSON:     `{"days":1,"days_relative":true}`,
			TimeJSON:     `{"hours":7,"minutes":0}`,
			Now:          time.Date(2020, 7, 17, 17, 1, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 18, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:         "10 января (текущая дата до 10 января) - без года подразумеваем, что это 10 января текущего года",
			DateJSON:     `{"days":10,"months":1}`,
			Now:          time.Date(2020, 1, 7, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 1, 10, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "10 января (текущая дата 17 января) - без года подразумеваем, что это 10 января в следующем году",
			DateJSON:     `{"days":10,"months":1}`,
			Now:          time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2021, 1, 10, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "10 января в 15:00 (сейчас 10 января 16:00) - предполагаем, что это будущее",
			DateJSON:     `{"days":10,"months":1}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 1, 10, 16, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2021, 1, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "20 числа (текущая дата в месяце меньше 20-ого) - без месяца подразумеваем, что это текущий месяц",
			DateJSON:     `{"days":20}`,
			Now:          time.Date(2020, 1, 17, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 1, 20, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "20 числа (текущая дата в месяце больше 20-ого) - без месяца подразумеваем, что это 20 число следующего месяца",
			DateJSON:     `{"days":20}`,
			Now:          time.Date(2020, 1, 27, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 2, 20, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В четверг в 15.00 (сейчас вторник) - подразумеваем, что это четверг на этой неделе",
			DateJSON:     `{"weekday":4}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 13, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В воскресенье в 15.00 (сейчас вторник) - подразумеваем, что это воскресенье на этой неделе",
			DateJSON:     `{"weekday":7}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 16, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			DateJSON:     `{"weekday":1}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В следующий понедельник в 15.00 (сейчас вторник) - подразумеваем, что это понедельник на следующей неделе",
			DateJSON:     `{"weekday":1,"weeks":1,"weeks_relative":true}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В следующую среду в 17:00 (сейчас вторник) - подразумеваем, что это среда на следующей неделе",
			DateJSON:     `{"weekday":3,"weeks":1,"weeks_relative":true}`,
			TimeJSON:     `{"hours":17,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 10, 12, 13, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 19, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В понедельник в 15.00 (сейчас понедельник 10.00) - подразумеваем, что это сегодня",
			DateJSON:     `{"weekday":1}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В понедельник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это следующий понедельник",
			DateJSON:     `{"weekday":1}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 17, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Во вторник в 15.00 (сейчас понедельник 18.00) - подразумеваем, что это завтра",
			DateJSON:     `{"weekday":2}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В четверг 8 сентября 2021 – такого дня нет. Использовать только число",
			DateJSON:     `{"weekday":4, "days": 8, "months": 9, "years": 2021}`,
			Now:          time.Date(2020, 7, 17, 17, 01, 25, 0, time.UTC),
			ExpectedTime: time.Date(2021, 9, 8, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 10 часов (сейчас уже 11) - подразумеваем, что это про 10 вечера сегодня",
			TimeJSON:     `{"hours":10,"minutes":0}`,
			Now:          time.Date(2020, 7, 17, 11, 1, 25, 0, time.UTC),
			ExpectedTime: time.Date(2020, 7, 17, 22, 0, 0, 0, time.UTC),
		},
		{
			Name:         "10 числа в 15.00 (сейчас 10-е число 10.00) - подразумеваем, что это сегодня",
			DateJSON:     `{"days":10}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 10, 10, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "10 числа в 15.00 (сейчас 10-е число 18.00) - подразумеваем, что это следующий месяц",
			DateJSON:     `{"days":10}`,
			TimeJSON:     `{"hours":15,"minutes":0}`,
			Now:          time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 9, 10, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сейчас - возвращаем текущее время",
			TimeJSON:     `{"seconds":0,"seconds_relative":true}`,
			Now:          time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 10, 18, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня - должна быть сегодняшняя дата",
			DateJSON:     `{"days":0,"days_relative":true}`,
			Now:          time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 10, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:         "Во вторник (сегодня вторник) - должна быть сегодняшняя дата",
			DateJSON:     `{"weekday":2}`,
			Now:          time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:         "В среду (сегодня вторник) - должна быть завтрашняя дата",
			DateJSON:     `{"weekday":3}`,
			Now:          time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 12, 0, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Через неделю - должна быть дата через неделю",
			DateJSON:     `{"weeks":1,"weeks_relative":true}`,
			Now:          time.Date(2020, 8, 11, 18, 15, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 18, 18, 15, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в два часа ночи (сейчас 11 вечера) - должно быть в прошлом",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В два часа ночи (сейчас 1 час ночи) - должно быть этой ночью",
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в семь часов утра (сейчас 11 вечера) - прошлое",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":7,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В семь часов утра (сейчас 11 вечера) - имеется ввиду завтра, будущее",
			TimeJSON:     `{"hours":7,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 12, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в семь часов утра (сейчас 5 утра) - имеется ввиду сегодня, будущее",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":7,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В семь часов утра (сейчас 5 утра) - имеется ввиду сегодня, будущее",
			TimeJSON:     `{"hours":7,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 7, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Завтра в два часа ночи (сейчас 11 вечера) - должно быть ночью на след день",
			DateJSON:     `{"days":1,"days_relative":true}`,
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 23, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Завтра в два часа ночи (сейчас 1 ночи) - должно быть ночью на след день",
			DateJSON:     `{"days":1,"days_relative":true}`,
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 12, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Вчера в два часа ночи - должно быть прошлое",
			DateJSON:     `{"days":-1,"days_relative":true}`,
			TimeJSON:     `{"hours":2,"minutes":0,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 10, 2, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 5 дня",
			TimeJSON:     `{"hours":5,"period":"pm"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 5 часов (сейчас время до 5 утра, поэтому думаем, что это 5 утра)",
			TimeJSON:     `{"hours":5}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 5 часов (сейчас время после 5 утра, поэтому думаем, что это 5 вечера)",
			TimeJSON:     `{"hours":5}`,
			Now:          time.Date(2020, 8, 11, 6, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 17 часов (сейчас время до 5 утра, но мы явно указали вечернее время)",
			TimeJSON:     `{"hours":17}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 17, 0, 0, 0, time.UTC),
		},
		{
			Name:         "В 5 часов утра",
			TimeJSON:     `{"hours":5,"period":"am"}`,
			Now:          time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 5, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в 1 час (сейчас 14:42) - прошлое",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":1,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 14, 42, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 1, 0, 0, 0, time.UTC),
		},
		{
			Name:         "Сегодня в 13 часов (сейчас 13:42) - завтра в час дня",
			DateJSON:     `{"days":0,"days_relative":true}`,
			TimeJSON:     `{"hours":13,"minutes":0}`,
			Now:          time.Date(2020, 8, 11, 13, 42, 0, 0, time.UTC),
			ExpectedTime: time.Date(2020, 8, 11, 13, 0, 0, 0, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var begemotDate BegemotDate
			var begemotTime BegemotTime

			if input.DateJSON != "" {
				err := begemotDate.FromValueString(input.DateJSON)
				assert.NoError(t, err)
			}

			if input.TimeJSON != "" {
				err := begemotTime.FromValueString(input.TimeJSON)
				assert.NoError(t, err)
			}

			dateTime := ParseBegemotDateAndTime(input.Now, &begemotDate, &begemotTime)
			assert.Equal(t, input.ExpectedTime, dateTime)
			assert.InDelta(t, 0, dateTime.Sub(input.ExpectedTime), float64(time.Second))
		})
	}
}

func TestDatetimeRangeFromValueString(t *testing.T) {
	inputs := []struct {
		Name              string
		DateTimeRangeJSON string
		Expected          BegemotDateTimeRange
	}{
		{
			Name:              "years",
			DateTimeRangeJSON: `{"end":{"years":20,"years_relative":true},"start":{"years":0,"years_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Years: ptr.Int(20),
				},
				Start: &DateTimeRangeInternal{
					Years: ptr.Int(0),
				},
			},
		},
		{
			Name:              "months",
			DateTimeRangeJSON: `{"end":{"months":3,"months_relative":true},"start":{"months":0,"months_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Months: ptr.Int(3),
				},
				Start: &DateTimeRangeInternal{
					Months: ptr.Int(0),
				},
			},
		},
		{
			Name:              "weeks",
			DateTimeRangeJSON: `{"end":{"weeks":0,"weeks_relative":true},"start":{"weeks":0,"weeks_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Weeks: ptr.Int(0),
				},
				Start: &DateTimeRangeInternal{
					Weeks: ptr.Int(0),
				},
			},
		},
		{
			Name:              "days",
			DateTimeRangeJSON: `{"end":{"days":7,"days_relative":true},"start":{"days":0,"days_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Days: ptr.Int(7),
				},
				Start: &DateTimeRangeInternal{
					Days: ptr.Int(0),
				},
			},
		},
		{
			Name:              "hybrid",
			DateTimeRangeJSON: `{"end":{"months":1,"months_relative":true,"weeks":2,"weeks_relative":true},"start":{"months":0,"months_relative":true,"weeks":0,"weeks_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Months: ptr.Int(1),
					Weeks:  ptr.Int(2),
				},
				Start: &DateTimeRangeInternal{
					Months: ptr.Int(0),
					Weeks:  ptr.Int(0),
				},
			},
		},
		{
			Name:              "hours",
			DateTimeRangeJSON: `{"end":{"hours":120,"hours_relative":true},"start":{"hours":0,"hours_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Hours: ptr.Int(120),
				},
				Start: &DateTimeRangeInternal{
					Hours: ptr.Int(0),
				},
			},
		},
		{
			Name:              "minutes",
			DateTimeRangeJSON: `{"end":{"minutes":16,"minutes_relative":true},"start":{"minutes":0,"minutes_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Minutes: ptr.Int(16),
				},
				Start: &DateTimeRangeInternal{
					Minutes: ptr.Int(0),
				},
			},
		},
		{
			Name:              "seconds",
			DateTimeRangeJSON: `{"end":{"seconds":3,"seconds_relative":true},"start":{"seconds":0,"seconds_relative":true}}`,
			Expected: BegemotDateTimeRange{
				End: &DateTimeRangeInternal{
					Seconds: ptr.Int(3),
				},
				Start: &DateTimeRangeInternal{
					Seconds: ptr.Int(0),
				},
			},
		},
		{
			// for some reason the phrase "праздники" is matched with sys.datetime_range
			Name:              "holidays",
			DateTimeRangeJSON: `{"end":{"holidays":true},"start":{"holidays":true}}`,
			Expected: BegemotDateTimeRange{
				End:   &DateTimeRangeInternal{},
				Start: &DateTimeRangeInternal{},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			var actual BegemotDateTimeRange
			var err error

			assert.NotPanics(t, func() {
				err = actual.FromValueString(input.DateTimeRangeJSON)
			})

			assert.NoError(t, err)
			assert.Equal(t, input.Expected, actual)
		})
	}
}

func TestSpecifyTime(t *testing.T) {
	clientTime := time.Date(2021, 9, 7, 16, 1, 0, 0, time.Local)

	hour := 1
	minute := 2
	second := 3

	zero := 0
	one := 1

	relativeDate := BegemotDate{
		Years:          &zero,
		Months:         &zero,
		Weeks:          &zero,
		Days:           &one,
		Weekday:        &zero,
		YearsRelative:  false,
		MonthsRelative: false,
		WeeksRelative:  false,
		DaysRelative:   true,
		DateRelative:   false,
	}

	year := clientTime.Year()
	month := int(clientTime.Month())
	day := clientTime.Day() + 1

	absoluteDate := BegemotDate{
		Years:  &year,
		Months: &month,
		Days:   &day,
	}

	inputs := []struct {
		Name         string
		BegemotDate  BegemotDate
		BegemotTime  BegemotTime
		ExpectedTime *BegemotTime
	}{
		{
			Name:        "Empty time and date",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{},
			ExpectedTime: newBegemotTimeNoChecks(clientTime.Hour(), clientTime.Minute(), clientTime.Second(),
				"", false, false, false, false),
		},
		{
			Name: "Only hour defined",
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, 0, 0,
				"", false, false, false, false),
		},
		{
			Name:        "Only minute defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           nil,
				Minutes:         &minute,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(0, minute, 0,
				"", false, false, false, false),
		},
		{
			Name:        "Hour and minute defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         &minute,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, minute, 0,
				"", false, false, false, false),
		},
		{
			Name:        "Full time defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         &minute,
				Seconds:         &second,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, minute, second,
				"", false, false, false, false),
		},
		{
			Name:        "Hour and second defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         &second,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, 0, second,
				"", false, false, false, false),
		},
		{
			Name:        "Minute and second defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           nil,
				Minutes:         &minute,
				Seconds:         &second,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(0, minute, second,
				"", false, false, false, false),
		},
		{
			Name:        "Minute and second defined",
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           nil,
				Minutes:         &minute,
				Seconds:         &second,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(0, minute, second,
				"", false, false, false, false),
		},
		{
			Name:        "Relative date, absolute time",
			BegemotDate: relativeDate,
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, 0, 0,
				"", false, false, false, false),
		},
		{
			Name:        "Relative date, no time",
			BegemotDate: relativeDate,
			BegemotTime: BegemotTime{},
			ExpectedTime: newBegemotTimeNoChecks(clientTime.Hour(), clientTime.Minute(), clientTime.Second(),
				"", false, false, false, false),
		},
		{
			Name:        "Relative time, no date", // begemotTime must not be changed by specifyTime
			BegemotDate: BegemotDate{},
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   true,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
			ExpectedTime: &BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   true,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
		},
		{
			Name:        "Relative time, relative date", // begemotTime must not be changed by specifyTime
			BegemotDate: relativeDate,
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   true,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
			ExpectedTime: &BegemotTime{
				Hours:           &hour,
				Minutes:         nil,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   true,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
		},
		{
			Name:        "Absolute time, absolute date",
			BegemotDate: absoluteDate,
			BegemotTime: BegemotTime{
				Hours:           &hour,
				Minutes:         &minute,
				Seconds:         nil,
				Period:          "",
				HoursRelative:   false,
				MinutesRelative: false,
				SecondsRelative: false,
				TimeRelative:    false,
			},
			ExpectedTime: newBegemotTimeNoChecks(hour, minute, 0,
				"", false, false, false, false),
		},
		{
			Name:        "Absolute date, no time", // btw, this is not allowed by the scenario
			BegemotDate: absoluteDate,
			BegemotTime: BegemotTime{},
			ExpectedTime: newBegemotTimeNoChecks(0, 0, 0,
				"", false, false, false, false),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			filledTime := specifyTime(clientTime, &input.BegemotTime, &input.BegemotDate)
			assert.Equal(t, input.ExpectedTime, filledTime)
		})
	}
}

// newBegemotTimeNoChecks creates a BegemotTime object with given parameters. It doesn't do any parameter validation
func newBegemotTimeNoChecks(hours int, minutes int, seconds int, period string, hoursRelative bool, minutesRelative bool,
	secondsRelative bool, timeRelative bool) *BegemotTime {
	return &BegemotTime{
		Hours:           &hours,
		Minutes:         &minutes,
		Seconds:         &seconds,
		Period:          period,
		HoursRelative:   hoursRelative,
		MinutesRelative: minutesRelative,
		SecondsRelative: secondsRelative,
		TimeRelative:    timeRelative,
	}
}
