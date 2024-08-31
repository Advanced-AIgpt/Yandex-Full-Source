package controller

import (
	"context"
	"fmt"
	"regexp"
	"sort"
	"strings"

	tb "gopkg.in/tucnak/telebot.v2" // todo: remove dep

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/bass"
	"a.yandex-team.ru/alice/amelie/pkg/iot"
	"a.yandex-team.ru/alice/amelie/pkg/re"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var (
	devicesText   = "Доступные устройства:"
	scenariosText = "Список сценариев:"

	devicesCommandRegexp   = newCommandRegexp("/devices", "/d")
	scenariosCommandRegexp = newCommandRegexp("/scenarios", "/sc")
	remoteCommandRegexp    = newCommandRegexp("/remote", "/r")
	sayCommandRegexp       = regexp.MustCompile(`^/(say|s)( (?P<text>.*))?$`)
	lightsCommandRegexp    = newCommandRegexp("/lights", "/l")

	devicesDevicePrefix   = "devices."
	devicesRemotePrefix   = "remote."
	devicesScenarioPrefix = "scenarios."
	devicesLightsPrefix   = "lights."

	devicesDeviceRegexp   = regexp.MustCompile(`^` + strings.ReplaceAll(devicesDevicePrefix, ".", `\.`) + `(?P<device_id>.+)$`)
	devicesRemoteRegexp   = regexp.MustCompile(`^` + strings.ReplaceAll(devicesRemotePrefix, ".", `\.`) + `(?P<action>.+)$`)
	devicesScenarioRegexp = regexp.MustCompile(`^` + strings.ReplaceAll(devicesScenarioPrefix, ".", `\.`) + `(?P<scenario_id>.+)$`)
	devicesLightsRegexp   = regexp.MustCompile(`^` + strings.ReplaceAll(devicesLightsPrefix, ".", `\.`) + `(?P<room_id>.+)$`)
)

const (
	callbackOk   = "🟢"
	callbackFail = "🔴"
)

type askMenuItem struct {
	title  string
	action string
}

var (
	remoteButtons = []askMenuItem{
		{title: "⬇️ ниже", action: "ниже"},
		{title: "⬆️ выше", action: "выше"},
		{title: "⬅️ назад", action: "назад"},
		{title: "➡️ дальше", action: "дальше"},
		{title: "⏸ пауза", action: "пауза"},
		{title: "▶️ продолжи", action: "продолжи"},
		{title: "⏪ перемотать назад", action: "перемотай на 10 секунд назад"},
		{title: "⏩ перемотать вперед", action: "перемотай на 10 секунд вперед"},
		{title: "🔉 тише", action: "тише"},
		{title: "🔊 громче", action: "громче"},
		{title: "❌ стоп", action: "хватит"},
		{title: "🏠 домой", action: "домой"},
	}
)

const (
	maxTTSTextLen = 1024
)

type deviceController struct {
	iotClientFactory   IoTClientFactory
	sessionInterceptor *interceptor.SessionInterceptor
	stateInterceptor   *interceptor.StateInterceptor
	bassClient         bass.Client
}

func (c *deviceController) getIoTClient(ctx context.Context) iot.Client {
	acc, _ := c.GetAccount(ctx)
	return c.iotClientFactory(acc.Token)
}

func (c *deviceController) registerStateCallback(stateInterceptor *interceptor.StateInterceptor) {
	stateInterceptor.RegisterCallback(&sayStateCallback{&sayCommand{c}})
}

func (c *deviceController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          "/remote",
		commandView:      "/*r*emote",
		shortDescription: "пульт управления мультимедиа",
		longDescription:  "пульт управления мультимедиа",
		groupName:        multimediaGroupName,
	})
	add(commandHelper{
		command:          "/say",
		commandView:      "/*s*ay",
		shortDescription: "сказать колонкой что-нибудь",
		longDescription:  "сказать колонкой что-нибудь",
		groupName:        multimediaGroupName,
	})
	add(commandHelper{
		command:          "/devices",
		commandView:      "/*d*evices",
		shortDescription: "управление колонками",
		longDescription:  "управление колонками",
		groupName:        iotGroupName,
	})
	add(commandHelper{
		command:          "/scenarios",
		commandView:      "/*sc*enarios",
		shortDescription: "управление сценариями",
		longDescription:  "управление сценариями",
		groupName:        iotGroupName,
	})
	add(commandHelper{
		command:          "/lights",
		commandView:      "/*l*ights",
		shortDescription: "управление светом",
		longDescription:  "управление светом",
		groupName:        iotGroupName,
	})
	add(commandHelper{
		command:          infoCommand,
		commandView:      infoCommandView,
		shortDescription: "служебная информация",
		longDescription:  "служебная информация",
		groupName:        iotGroupName,
	})
}

type sayCommand struct {
	*deviceController
}

func (s *sayCommand) say(ctx context.Context, bot telegram.Bot, text string) {
	if len(text) > maxTTSTextLen {
		_, _ = bot.Reply(ctx, "Слишком длинный текст")
		return
	}
	acc, _ := s.deviceController.GetAccount(ctx)
	if err := s.bassClient.PlayTTS(ctx, acc.ID, acc.DeviceID, text); err != nil {
		_, _ = bot.Reply(ctx, "Что-то у меня не получилось")
		return
	}
	_, _ = bot.Reply(ctx, "Попросила Алису в колонке это сказать")
}

func (s *sayCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	if !s.deviceController.EnsureActiveDevice(ctx, bot) {
		return nil
	}
	groups := re.MatchNamedGroups(sayCommandRegexp, msg.Text)
	text, ok := groups["text"]
	if !ok {
		// TODO: add user message
		return nil
	}
	text = strings.Trim(text, " ")
	if text == "" {
		s.stateInterceptor.SetState(ctx, sayInputState)
		_, _ = bot.Reply(ctx, "Что нужно сказать?") // todo: add state
		return nil
	}
	s.say(ctx, bot, text)
	return nil
}

func (s *sayCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return sayCommandRegexp.MatchString(msg.Text)
}

type sayStateCallback struct {
	*sayCommand
}

func (s *sayStateCallback) IsRelevant(ctx context.Context, state string) bool {
	return state == sayInputState
}

func (s *sayStateCallback) Handle(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}, state string) (fallThrough bool) {
	if eventType != telegram.TextEvent {
		s.stateInterceptor.SetState(ctx, sayInputState)
		_, _ = bot.Reply(ctx, "Нужно ввести текст")
		return
	}
	s.say(ctx, bot, telegram.AsMessage(event).Text)
	return false
}

func (c *deviceController) AskAlice(ctx context.Context, bot telegram.Bot, text string) error {
	if !c.EnsureActiveDevice(ctx, bot) {
		return nil
	}
	acc, _ := c.GetAccount(ctx)
	return c.bassClient.SendText(ctx, acc.ID, acc.DeviceID, text)
}

func (c *deviceController) EnsureActiveDevice(ctx context.Context, bot telegram.Bot) bool {
	acc, err := c.GetAccount(ctx)
	if err != nil {
		// what?
		return false
	}
	if acc.DeviceID == "" {
		_, _ = bot.Reply(ctx, "Для того, чтобы выполнить эту команду, необходимо выбрать устройство с помощью команды /devices")
		return false
	}
	return true
}

func (c *deviceController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&devicesCommand{c})
	commandInterceptor.AddTextCommand(&sayCommand{c})
	commandInterceptor.AddTextCommand(&remoteCommand{c})
	commandInterceptor.AddTextCommand(&scenariosCommand{c})
	commandInterceptor.AddTextCommand(&lightsCommand{c})
	commandInterceptor.AddTextCommand(&infoCommandHandler{c})

	commandInterceptor.AddCallbackCommand(&scenariosCallback{c})
	commandInterceptor.AddCallbackCommand(&devicesDeviceCallback{c})
	commandInterceptor.AddCallbackCommand(&remoteCallback{c})
	commandInterceptor.AddCallbackCommand(&lightsCallback{c})
}

func (c *deviceController) GetAccount(ctx context.Context) (model.Account, error) {
	return c.sessionInterceptor.GetSession(ctx).User.GetActiveAccount()
}

type devicesCommand struct {
	*deviceController
}

type deviceInfo struct {
	name     string
	room     string
	deviceID string
	isActive bool
}

type roomsByName iot.Rooms

func (a roomsByName) Len() int {
	return len(a)
}

func (a roomsByName) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

func (a roomsByName) Less(i, j int) bool {
	return a[i].Name < a[j].Name
}

type scenariosByName iot.Scenarios

func (a scenariosByName) Len() int {
	return len(a)
}

func (a scenariosByName) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

func (a scenariosByName) Less(i, j int) bool {
	return a[i].Name < a[j].Name
}

type byRoomThenByNameThenByDeviceID []deviceInfo
type byActiveThenByRoomThenByNameThenByDeviceID []deviceInfo

func (a byRoomThenByNameThenByDeviceID) Len() int {
	return len(a)
}
func (a byActiveThenByRoomThenByNameThenByDeviceID) Len() int {
	return byRoomThenByNameThenByDeviceID(a).Len()
}
func (a byRoomThenByNameThenByDeviceID) Less(i, j int) bool {
	if a[i].room != a[j].room {
		return a[i].room < a[j].room
	}
	if a[i].name != a[j].name {
		return a[i].name < a[j].name
	}
	return a[i].deviceID < a[j].deviceID
}
func (a byActiveThenByRoomThenByNameThenByDeviceID) Less(i, j int) bool {
	if a[i].isActive != a[j].isActive {
		return a[i].isActive
	}
	return byRoomThenByNameThenByDeviceID(a).Less(i, j)
}

func (a byRoomThenByNameThenByDeviceID) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

func (a byActiveThenByRoomThenByNameThenByDeviceID) Swap(i, j int) {
	(byRoomThenByNameThenByDeviceID)(a).Swap(i, j)
}

func (cmd *devicesCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	buttons, err := cmd.makeButtonsWithSpeakers(ctx)
	if err != nil {
		if userErr, ok := err.(*userError); ok {
			return userErr.reply(ctx, bot)
		}
		return err
	}
	_, _ = bot.Reply(ctx, devicesText, &tb.ReplyMarkup{
		InlineKeyboard: buttons,
	})
	// todo: add more button
	return nil
}

func (cmd *devicesCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return devicesCommandRegexp.MatchString(msg.Text)
}

func (c *deviceController) getSpeakers(ctx context.Context) ([]deviceInfo, error) {
	acc, _ := c.GetAccount(ctx)
	userInfo, err := c.getIoTClient(ctx).GetUserInfo(ctx)
	if err != nil {
		return nil, &userError{
			msg: "Не получилось загрузить список устройств",
		}
	}
	if len(userInfo.Devices) == 0 {
		return nil, &userError{
			msg: "Похоже, что у Вас еще нет устройств",
		}
	}
	var devices []deviceInfo
	for _, device := range userInfo.Devices {
		if !strings.HasPrefix(device.Type, "devices.types.smart_speaker") {
			continue
		}
		var info deviceInfo
		info.name = device.Name
		info.deviceID = device.GetDeviceID()
		if room, err := userInfo.Rooms.Find(device.Room); err == nil {
			info.room = room.Name
		}
		info.isActive = info.deviceID == acc.DeviceID
		devices = append(devices, info)
	}
	sort.Sort(byRoomThenByNameThenByDeviceID(devices))
	return devices, nil
}

func (c *deviceController) makeButtonsWithSpeakers(ctx context.Context) ([][]tb.InlineButton, error) {
	devices, err := c.getSpeakers(ctx)
	if err != nil {
		return nil, err
	}
	var buttons [][]tb.InlineButton
	for _, device := range devices {
		buttons = append(buttons, []tb.InlineButton{
			{
				Text: fmt.Sprintf("%s%s%s", func() string {
					if device.isActive {
						return "🟢 "
					}
					return ""
				}(), device.name, func() string {
					if device.room != "" {
						return fmt.Sprintf(" (%s)", device.room)
					}
					return ""
				}()),
				Data: fmt.Sprintf("%s%s", devicesDevicePrefix, device.deviceID),
			},
		})
	}
	return buttons, nil
}

func (c *deviceController) makeButtonsWithLights(ctx context.Context) ([][]tb.InlineButton, error) {
	userInfo, err := c.getIoTClient(ctx).GetUserInfo(ctx)
	if err != nil {
		return nil, &userError{
			msg: "Не получилось загрузить список лампочек",
		}
	}
	if len(userInfo.Devices) == 0 {
		return nil, &userError{
			msg: "Похоже, что у Вас еще нет лампочек",
		}
	}
	roomIds := map[string]iot.Room{}
	for _, device := range userInfo.Devices {
		if !strings.HasPrefix(device.Type, "devices.types.light") {
			continue
		}
		if room, err := userInfo.Rooms.Find(device.Room); err == nil {
			roomIds[room.ID] = room
		}
	}
	if len(roomIds) == 0 {
		return nil, &userError{
			msg: "Похоже, что у Вас еще нет лампочек",
		}
	}
	rooms := func() iot.Rooms {
		var rooms iot.Rooms
		for _, room := range roomIds {
			rooms = append(rooms, room)
		}
		return rooms
	}()
	sort.Sort(roomsByName(rooms))
	var buttons [][]tb.InlineButton
	for _, room := range rooms {
		buttons = append(buttons, []tb.InlineButton{
			{
				Text: fmt.Sprintf("💡 свет в комнате %s", room.Name),
				Data: fmt.Sprintf("%s%s", devicesLightsPrefix, room.ID),
			},
		})
	}
	return buttons, nil
}

func (c *deviceController) makeButtonsWithScenarios(ctx context.Context) ([][]tb.InlineButton, error) {
	userInfo, err := c.getIoTClient(ctx).GetUserInfo(ctx)
	if err != nil {
		return nil, &userError{
			msg: "Не получилось загрузить список сценариев",
		}
	}
	sourceScenarios := userInfo.Scenarios
	if len(sourceScenarios) == 0 {
		return nil, &userError{
			msg: "Похоже, что у Вас еще нет сценариев",
			button: &tb.InlineButton{
				Text: "Как создать сценарий?",
				// TODO: do we need ATM?
				URL: "https://yandex.ru/support/smart-home/several/scenarios.html",
			},
		}
	}
	var scenarios iot.Scenarios
	for _, scenario := range sourceScenarios {
		if scenario.IsActive {
			scenarios = append(scenarios, scenario)
		}
	}
	if len(scenarios) == 0 {
		return nil, &userError{
			msg: "Похоже, что все Ваши сценарии выключены",
			button: &tb.InlineButton{
				Text: "Что значит выключены?",
				URL:  "https://yandex.ru/support/smart-home/several/scenarios.html#off-scenarios",
			},
		}
	}
	sort.Sort(scenariosByName(scenarios))
	var buttons [][]tb.InlineButton
	for _, scenario := range scenarios {
		buttons = append(buttons, []tb.InlineButton{
			{
				Text: "▶ " + scenario.Name,
				Data: fmt.Sprintf("%s%s", devicesScenarioPrefix, scenario.ID),
			},
		})
	}
	return buttons, nil
}

type devicesDeviceCallback struct {
	*deviceController
}

func (c *devicesDeviceCallback) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	groups := re.MatchNamedGroups(devicesDeviceRegexp, cb.Data)
	deviceID, ok := groups["device_id"]
	if !ok {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return nil
	}
	c.sessionInterceptor.GetSession(ctx).User.Accounts[0].DeviceID = deviceID
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	buttons, err := c.deviceController.makeButtonsWithSpeakers(ctx)
	if err != nil {
		if userErr, ok := err.(*userError); ok {
			return userErr.reply(ctx, bot)
		}
		return err // TODO: improve UX
	}
	_, err = bot.Edit(ctx, (*telegram.Message)(cb.Message), devicesText, &tb.ReplyMarkup{InlineKeyboard: buttons})
	return err
}

func (c *devicesDeviceCallback) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return devicesDeviceRegexp.MatchString(cb.Data)
}

type remoteCommand struct {
	*deviceController
}

func (r *remoteCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	var buttons [][]tb.InlineButton
	var row []tb.InlineButton
	for _, item := range remoteButtons {
		// TODO: it's better to use hash in order to allow use long utterances
		data := devicesRemotePrefix + item.action
		if len([]byte(data)) > 64 {
			return fmt.Errorf("data is too long: %s", data)
		}
		row = append(row, tb.InlineButton{
			Text: item.title,
			Data: data,
		})
		if len(row) == 2 {
			buttons = append(buttons, row)
			row = nil
		}
	}
	if len(row) != 0 {
		buttons = append(buttons, row)
		row = nil
	}
	_, _ = bot.Reply(ctx, "Вот, что я могу:", &tb.ReplyMarkup{InlineKeyboard: buttons})
	return nil
}

func (r *remoteCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return remoteCommandRegexp.MatchString(msg.Text)
}

type remoteCallback struct {
	*deviceController
}

func (r *remoteCallback) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	groups := re.MatchNamedGroups(devicesRemoteRegexp, cb.Data)
	action, ok := groups["action"]
	if !ok {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return nil
	}
	if err := r.deviceController.AskAlice(ctx, bot, action); err != nil {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return err
	}
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	return nil
}

func (r *remoteCallback) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return devicesRemoteRegexp.MatchString(cb.Data)
}

type scenariosCommand struct {
	*deviceController
}

func (cmd *scenariosCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return scenariosCommandRegexp.MatchString(msg.Text)
}

func (cmd *scenariosCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	buttons, err := cmd.makeButtonsWithScenarios(ctx)
	if err != nil {
		if userErr, ok := err.(*userError); ok {
			return userErr.reply(ctx, bot)
		}
		return err
	}
	_, _ = bot.Reply(ctx, scenariosText, &tb.ReplyMarkup{
		InlineKeyboard: buttons,
	})
	// todo: add more button
	return err
}

type scenariosCallback struct {
	*deviceController
}

func (r *scenariosCallback) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	groups := re.MatchNamedGroups(devicesScenarioRegexp, cb.Data)
	scenarioID, ok := groups["scenario_id"]
	if !ok {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return nil
	}
	if err := r.getIoTClient(ctx).RunScenario(ctx, scenarioID); err != nil {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return err
	}
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	return nil
}

func (r *scenariosCallback) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return devicesScenarioRegexp.MatchString(cb.Data)
}

type lightsCommand struct {
	*deviceController
}

func (cmd *lightsCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return lightsCommandRegexp.MatchString(msg.Text)
}

func (cmd *lightsCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	buttons, err := cmd.makeButtonsWithLights(ctx)
	if err != nil {
		if userErr, ok := err.(*userError); ok {
			return userErr.reply(ctx, bot)
		}
		return err
	}
	_, _ = bot.Reply(ctx, "Доступные комнаты", &tb.ReplyMarkup{
		InlineKeyboard: buttons,
	})
	// todo: add more button
	return err
}

type lightsCallback struct {
	*deviceController
}

func (r *lightsCallback) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	groups := re.MatchNamedGroups(devicesLightsRegexp, cb.Data)
	roomID, ok := groups["room_id"]
	if !ok {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail,
		})
		return nil
	}
	userInfo, err := r.getIoTClient(ctx).GetUserInfo(ctx)
	if err != nil {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail, // не получилось загрузить список устройств
		})
		return err
	}
	room, err := userInfo.Rooms.Find(roomID)
	if err != nil {
		_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
			CallbackID: cb.ID,
			Text:       callbackFail, // не получилось найти комнату
		})
		return err
	}
	_ = r.deviceController.AskAlice(ctx, bot, fmt.Sprintf("свет %s", room.Name))
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	return nil
}

func (r *lightsCallback) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return devicesLightsRegexp.MatchString(cb.Data)
}

type infoCommandHandler struct {
	*deviceController
}

func (h *infoCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	speakers, err := h.getSpeakers(ctx)
	if err != nil {
		if userErr, ok := err.(*userError); ok {
			return userErr.reply(ctx, bot)
		}
		return err
	}
	// TODO: do we need lights or other devices here?
	rooms := map[string][]deviceInfo{}
	getRoomName := func(s string) string {
		if len(s) > 0 {
			return s
		}
		return "Без комнаты"
	}
	for _, speaker := range speakers {
		rooms[getRoomName(speaker.room)] = append(rooms[getRoomName(speaker.room)], speaker)
	}
	var lines []string
	for roomName, devices := range rooms {
		lines = append(lines, fmt.Sprintf("*%s*", telegram.EscapeMD(roomName)))
		for _, device := range devices {
			lines = append(lines, fmt.Sprintf("%s: `%s`", telegram.EscapeMD("– "+device.name),
				telegram.EscapeMDCode(device.deviceID)))
		}
	}
	_, _ = bot.Reply(ctx, strings.Join(lines, "\n"), tb.ModeMarkdownV2)
	return nil
}

func (h *infoCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return infoCommandRegexp.MatchString(msg.Text)
}
