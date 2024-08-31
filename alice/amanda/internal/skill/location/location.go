package location

import (
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

var (
	_yandexOfficeLocation = "Yandex Office location"
)

func RegistrySkill(c common.Controller) {
	c.AddCommand(common.NewCommand("location"), location)
	common.AddInputState(c, "setlocation", setLocation, setLocationArg)
	common.AddInputStateWithHelpText(c, "setregionid", "Введите RegionID или `none` для сброса", setRegionIDArg)
	c.AddLocationInterceptor(onLocation)
	c.AddCallbackWithArgs(regexp.MustCompile(`^location\.set\.(.+)$`), func(ctx app.Context, cb *telebot.Callback, args []string) {
		_ = ctx.Delete(cb.Message)
		setLocationArg(ctx, cb.Message, args[0])
	})
}

func setRegionIDArg(ctx app.Context, msg *telebot.Message, arg string) {
	id, err := strconv.Atoi(strings.Trim(arg, " "))
	if err != nil {
		ctx.GetSettings().ResetRegionID()
	} else {
		ctx.GetSettings().SetRegionID(int32(id))
	}
	value := formatRegionID(ctx.GetSettings().GetRegionID())
	if value == "" {
		value = "значение по умолчанию"
	}
	_, _ = ctx.SendMD(common.MakeChangeText("RegionID", value))
}

func setLocation(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(
		common.EscapeMD(`Введите местоположение в формате "<lat> <lon>" или отправьте его как Telegram Location`),
		&telebot.ReplyMarkup{
			ReplyKeyboard: [][]telebot.ReplyButton{{{
				Text: _yandexOfficeLocation,
			}}},
			ResizeReplyKeyboard: true,
			OneTimeKeyboard:     true,
		},
	)
}

func onLocation(ctx app.Context, msg *telebot.Message) (proceed bool) {
	_, _ = ctx.SendMD("Хотите установить эту локацию?", &telebot.ReplyMarkup{
		InlineKeyboard: [][]telebot.InlineButton{
			{
				{
					Text: "Да",
					Data: fmt.Sprintf("location.set.%f %f", msg.Location.Lat, msg.Location.Lng),
				},
			},
		},
		OneTimeKeyboard:     true,
		ReplyKeyboardRemove: true,
	})
	return false
}

func setLocationArg(ctx app.Context, msg *telebot.Message, arg string) {
	if msg.Location != nil {
		ctx.GetSettings().GetLocation().Longitude = float64(msg.Location.Lng)
		ctx.GetSettings().GetLocation().Latitude = float64(msg.Location.Lat)
		location(ctx, msg)
		return
	}
	if arg == _yandexOfficeLocation {
		ctx.GetSettings().GetLocation().Latitude = app.YandexOfficeLocation.Latitude
		ctx.GetSettings().GetLocation().Longitude = app.YandexOfficeLocation.Longitude
		location(ctx, msg)
		return
	}
	values := strings.SplitN(arg, " ", 2)
	var latErr, lonErr error
	var lat, lon float64
	nValues := len(values)
	if nValues == 2 {
		lat, latErr = strconv.ParseFloat(values[0], 64)
		lon, lonErr = strconv.ParseFloat(values[1], 64)
	}
	if nValues != 2 || latErr != nil || lonErr != nil {
		_, _ = ctx.SendMD("Не удалось распознать lat и lon")
		return
	}
	ctx.GetSettings().GetLocation().Latitude = lat
	ctx.GetSettings().GetLocation().Longitude = lon
	location(ctx, msg)
}

func GetInfo(ctx app.Context) string {
	loc := ctx.GetSettings().GetLocation()
	_, _ = ctx.Send(&telebot.Location{
		Lat: float32(loc.Latitude),
		Lng: float32(loc.Longitude),
	})
	return strings.Join([]string{
		common.FormatFieldValueMD("Latitude", fmt.Sprintf("%f", loc.Latitude)),
		common.FormatFieldValueMD("Longitude", fmt.Sprintf("%f", loc.Longitude)),
		common.FormatFieldValueMD("RegionID", formatRegionID(ctx.GetSettings().GetRegionID())),
	}, "\n")
}

func formatRegionID(id *int32) string {
	if id != nil {
		return strconv.Itoa(int(*id))
	}
	return ""
}

func location(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(fmt.Sprintf("%s\n%s",
		common.AddHeader(ctx, "Местоположение", "setlocation", "setregionid"), GetInfo(ctx)))
}
