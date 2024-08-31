package nlg

import (
	"fmt"
	"time"

	"a.yandex-team.ru/alice/library/go/nlg"
)

var monthsMap = map[time.Month]string{
	time.January:   "января",
	time.February:  "февраля",
	time.March:     "марта",
	time.April:     "апреля",
	time.May:       "мая",
	time.June:      "июня",
	time.July:      "июля",
	time.August:    "августа",
	time.September: "сентября",
	time.October:   "октября",
	time.November:  "ноября",
	time.December:  "декабря",
}

func DelayedAction(now time.Time, scheduleTime time.Time, userLocation *time.Location) libnlg.NLG {
	userLocalNowTime := now.In(userLocation)
	userLocalScheduledTime := scheduleTime.In(userLocation)

	date := formatDate(userLocalNowTime, userLocalScheduledTime)
	time := formatTime(userLocalScheduledTime.Hour(), userLocalScheduledTime.Minute())

	reply := fmt.Sprintf("Хорошо, запомнила: сделаю %s в %s", date, time)

	return libnlg.FromVariants([]string{reply})
}

func OnOffStateIntervalAction(onOffValue bool, now time.Time, scheduleTime time.Time, userLocation *time.Location) libnlg.NLG {
	userLocalNowTime := now.In(userLocation)
	userLocalScheduledTime := scheduleTime.In(userLocation)

	date := formatDate(userLocalNowTime, userLocalScheduledTime)
	time := formatTime(userLocalScheduledTime.Hour(), userLocalScheduledTime.Minute())

	var reply string

	if onOffValue {
		reply = fmt.Sprintf("Готово. По вашей просьбе выключу %s в %s", date, time)
	} else {
		reply = fmt.Sprintf("Готово. По вашей просьбе включу %s в %s", date, time)
	}

	return libnlg.FromVariants([]string{reply})
}

func CommonIntervalAction(now time.Time, scheduleTime time.Time, userLocation *time.Location) libnlg.NLG {
	userLocalNowTime := now.In(userLocation)
	userLocalScheduledTime := scheduleTime.In(userLocation)

	date := formatDate(userLocalNowTime, userLocalScheduledTime)
	time := formatTime(userLocalScheduledTime.Hour(), userLocalScheduledTime.Minute())

	reply := fmt.Sprintf("Хорошо, верну обратно %s в %s", date, time)

	return libnlg.FromVariants([]string{reply})
}

func formatDate(userLocalNowTime, userLocalScheduledTime time.Time) string {
	var date string
	switch {
	case dateIsEqual(userLocalNowTime, userLocalScheduledTime):
		date = "сегодня"
	case dateIsEqual(userLocalNowTime.Add(24*time.Hour), userLocalScheduledTime):
		date = "завтра"
	case dateIsEqual(userLocalNowTime.Add(2*24*time.Hour), userLocalScheduledTime):
		date = "послезавтра"
	default:
		_, month, day := userLocalScheduledTime.Date()
		date = fmt.Sprintf("%d %s", day, monthsMap[month])
	}

	return date
}

func formatTime(hour, minute int) string {
	var result string

	switch {
	case hour == 1 || hour == 21:
		result += fmt.Sprintf("%d час ", hour)
	case hour >= 2 && hour <= 4 || hour >= 22 && hour <= 24:
		result += fmt.Sprintf("%d часа ", hour)
	default:
		result += fmt.Sprintf("%d часов ", hour)
	}

	if minute >= 10 && minute < 20 {
		result += fmt.Sprintf("%d минут", minute)
	} else {
		switch minute % 10 {
		case 1:
			result += fmt.Sprintf("%d минуту", minute)
		case 2, 3, 4:
			result += fmt.Sprintf("%d минуты", minute)
		default:
			result += fmt.Sprintf("%d минут", minute)
		}
	}

	return result
}

func dateIsEqual(a, b time.Time) bool {
	if a.Year() != b.Year() {
		return false
	}
	if a.Month() != b.Month() {
		return false
	}
	if a.Day() != b.Day() {
		return false
	}
	return true
}

var DelayedActionCancel = libnlg.NLG{
	libnlg.NewAssetWithText("Передумали? Ладно. Отменила этот план!"),
}

var AllDelayedActionsCancel = libnlg.NLG{
	libnlg.NewAssetWithText("Нет проблем. Ничего не буду делать - отдохну."),
}
