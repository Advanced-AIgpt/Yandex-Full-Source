package telebot

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"

	"github.com/go-resty/resty/v2"
	tb "gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amelie/pkg/logging"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ServiceBot interface {
	telegram.Bot
	SetWebhook(ctx context.Context, w *tb.Webhook) error
	ProcessUpdate(ctx context.Context, upd tb.Update)
	StartLongPolling(updatesChan chan tb.Update) error
	StopLongPolling()
}

type bot struct {
	tbBot               *tb.Bot
	app                 telegram.App
	client              *resty.Client
	stopLongPollingChan chan struct{}
}

func (b *bot) SetCommands(ctx context.Context, commands []telegram.Command) error {
	data, _ := json.Marshal(commands)
	params := map[string]string{
		"commands": string(data),
	}
	_, err := b.callAPI(ctx, "setMyCommands", params)
	return err
}

func (b *bot) Send(ctx context.Context, to telegram.Recipient, what interface{}, options ...interface{}) (*telegram.Message, error) {
	if to == nil {
		return nil, tb.ErrBadRecipient
	}

	sendOpts := extractOptions(options)

	switch object := what.(type) {
	case string:
		msg, err := b.sendText(ctx, tb.Recipient(to), object, sendOpts)
		return (*telegram.Message)(msg), err
	case tb.Sendable:
		// TODO: use own types!
		msg, err := b.tbBot.Send(to, what, options)
		return (*telegram.Message)(msg), err
	default:
		return nil, tb.ErrUnsupportedWhat
	}
}

func (b *bot) Respond(ctx context.Context, cb *telegram.Callback, response ...*telegram.CallbackResponse) error {
	var resp *telegram.CallbackResponse
	if response == nil {
		resp = &telegram.CallbackResponse{}
	} else {
		resp = response[0]
	}

	resp.CallbackID = cb.ID
	_, err := b.callAPI(ctx, "answerCallbackQuery", resp)
	return err
}

func (b *bot) Edit(ctx context.Context, msg *telegram.Message, what interface{}, options ...interface{}) (*telegram.Message, error) {
	var (
		method string
		params = make(map[string]string)
	)

	switch v := what.(type) {
	case string:
		method = "editMessageText"
		params["text"] = v
	case telegram.Location:
		method = "editMessageLiveLocation"
		params["latitude"] = fmt.Sprintf("%f", v.Lat)
		params["longitude"] = fmt.Sprintf("%f", v.Lng)
	default:
		return nil, tb.ErrUnsupportedWhat
	}

	msgID, chatID := (*tb.Message)(msg).MessageSig()

	if chatID == 0 { // if inline message
		params["inline_message_id"] = msgID
	} else {
		params["chat_id"] = strconv.FormatInt(chatID, 10)
		params["message_id"] = msgID
	}

	sendOpts := extractOptions(options)
	embedSendOptions(params, sendOpts)

	data, err := b.callAPI(ctx, method, params)
	if err != nil {
		return nil, err
	}
	r, err := extractMessage(data)
	if err != nil {
		return nil, err
	}
	return (*telegram.Message)(r), nil
}

func (b *bot) Delete(ctx context.Context, msg *telegram.Message) error {
	params := map[string]string{
		"chat_id":    strconv.FormatInt(msg.Chat.ID, 10),
		"message_id": strconv.Itoa(msg.ID),
	}
	_, err := b.callAPI(ctx, "deleteMessage", params)
	return err
}

func (b *bot) Reply(ctx context.Context, what interface{}, options ...interface{}) (*telegram.Message, error) {
	meta := telegram.GetEventMeta(ctx)
	return b.Send(ctx, &tb.User{
		Username: meta.Username,
		ID:       int(meta.UserID),
	}, what, options...)
}

func (b *bot) StartLongPolling(updatesChan chan tb.Update) error {
	if b.tbBot.Poller == nil {
		return fmt.Errorf("long lopping error: can't start without a poller")
	}
	b.tbBot.Poller.Poll(b.tbBot, updatesChan, b.stopLongPollingChan)
	return nil
}

func (b *bot) StopLongPolling() {
	b.stopLongPollingChan <- struct{}{}
}

func (b *bot) sendText(ctx context.Context, to tb.Recipient, text string, sendOpts *tb.SendOptions) (*tb.Message, error) {
	params := map[string]string{
		"chat_id": to.Recipient(),
		"text":    text,
	}
	embedSendOptions(params, sendOpts)
	data, err := b.callAPI(ctx, "sendMessage", params)
	if err != nil {
		return nil, err
	}
	return extractMessage(data)
}

func (b *bot) GetMe() *tb.User {
	return b.tbBot.Me
}

func NewBot(b *tb.Bot, app telegram.App, client *resty.Client) (ServiceBot, error) {
	return &bot{
		tbBot:               b,
		app:                 app,
		client:              client,
		stopLongPollingChan: make(chan struct{}, 1),
	}, nil
}

func (b *bot) callAPI(ctx context.Context, method string, payload interface{}) ([]byte, error) {
	url := b.tbBot.URL + "/bot" + b.tbBot.Token + "/" + method
	req := b.client.R().
		SetHeader("Content-Type", "application/json").
		SetContext(ctx).
		SetBody(payload)
	// TODO: add rps limiter
	resp, err := req.Post(url)
	if err != nil {
		return nil, fmt.Errorf("telegram call api error: %w", err)
	}
	data := resp.Body()
	if resp.IsError() {
		return nil, fmt.Errorf("telegram api error: status_code=%d body=%s", resp.StatusCode(), data)
	}
	// returning data as well
	return data, extractOk(data)
}

func (b *bot) SetWebhook(ctx context.Context, w *tb.Webhook) error {
	// TODO: use own types!
	return b.tbBot.SetWebhook(w)
}

func (b *bot) ProcessUpdate(ctx context.Context, upd tb.Update) {
	if upd.Message != nil {
		m := (*telegram.Message)(upd.Message)

		if m.PinnedMessage != nil {
			b.app.OnPinned(ctx, b, m)
			return
		}

		// Commands
		if m.Text != "" {
			b.app.OnText(ctx, b, m)
			return
		}

		if b.handleMedia(ctx, (*tb.Message)(m)) {
			return
		}

		if m.Invoice != nil {
			b.app.OnInvoice(ctx, b, m)
			return
		}

		if m.Payment != nil {
			b.app.OnPayment(ctx, b, m)
			return
		}

		wasAdded := (m.UserJoined != nil && m.UserJoined.ID == b.tbBot.Me.ID) ||
			(m.UsersJoined != nil && isUserInList(b.tbBot.Me, m.UsersJoined))
		if m.GroupCreated || m.SuperGroupCreated || wasAdded {
			b.app.OnAddedToGroup(ctx, b, m)
			return
		}

		if m.UserJoined != nil {
			b.app.OnUserJoined(ctx, b, m)
			return
		}

		if m.UsersJoined != nil {
			for _, user := range m.UsersJoined {
				m.UserJoined = &user
				b.app.OnUserJoined(ctx, b, m)
			}
			return
		}

		if m.UserLeft != nil {
			b.app.OnUserLeft(ctx, b, m)
			return
		}

		if m.NewGroupTitle != "" {
			b.app.OnNewGroupTitle(ctx, b, m)
			return
		}

		if m.NewGroupPhoto != nil {
			b.app.OnNewGroupPhoto(ctx, b, m)
			return
		}

		if m.GroupPhotoDeleted {
			b.app.OnGroupPhotoDeleted(ctx, b, m)
			return
		}

		if m.MigrateTo != 0 {
			ctxlog.Errorf(ctx, logging.ServiceLogger(ctx), "unhandled OnMigration")
			return
		}
	}

	if upd.EditedMessage != nil {
		b.app.OnEdited(ctx, b, (*telegram.Message)(upd.EditedMessage))
		return
	}

	if upd.ChannelPost != nil {
		m := upd.ChannelPost

		if m.PinnedMessage != nil {
			b.app.OnPinned(ctx, b, (*telegram.Message)(m))
			return
		}

		b.app.OnChannelPost(ctx, b, (*telegram.Message)(upd.ChannelPost))
		return
	}

	if upd.EditedChannelPost != nil {
		b.app.OnEditedChannelPost(ctx, b, (*telegram.Message)(upd.EditedChannelPost))
		return
	}

	if upd.Callback != nil {
		if upd.Callback.Data != "" {
			if upd.Callback.MessageID != "" {
				upd.Callback.Message = &tb.Message{
					// InlineID indicates that message
					// is inline so we have only its id
					InlineID: upd.Callback.MessageID,
				}
			}
		}
		b.app.OnCallback(ctx, b, (*telegram.Callback)(upd.Callback))
		return
	}

	if upd.Query != nil {
		b.app.OnQuery(ctx, b, (*telegram.Query)(upd.Query))
		return
	}

	if upd.ChosenInlineResult != nil {
		b.app.OnChosenInlineResult(ctx, b, (*telegram.ChosenInlineResult)(upd.ChosenInlineResult))
		return
	}

	if upd.ShippingQuery != nil {
		b.app.OnShipping(ctx, b, (*telegram.ShippingQuery)(upd.ShippingQuery))
		return
	}

	if upd.PreCheckoutQuery != nil {
		b.app.OnCheckout(ctx, b, (*telegram.PreCheckoutQuery)(upd.PreCheckoutQuery))
		return
	}

	if upd.Poll != nil {
		b.app.OnPoll(ctx, b, (*telegram.Poll)(upd.Poll))
		return
	}

	if upd.PollAnswer != nil {
		b.app.OnPollAnswer(ctx, b, (*telegram.PollAnswer)(upd.PollAnswer))
		return
	}

	ctxlog.Errorf(ctx, logging.ServiceLogger(ctx), "unhandled update: %+v", upd)
}

func (b *bot) handleMedia(ctx context.Context, m *tb.Message) bool {
	switch {
	case m.Photo != nil:
		b.app.OnPhoto(ctx, b, (*telegram.Message)(m))
	case m.Voice != nil:
		b.app.OnVoice(ctx, b, (*telegram.Message)(m))
	case m.Audio != nil:
		b.app.OnAudio(ctx, b, (*telegram.Message)(m))
	case m.Animation != nil:
		b.app.OnAnimation(ctx, b, (*telegram.Message)(m))
	case m.Document != nil:
		b.app.OnDocument(ctx, b, (*telegram.Message)(m))
	case m.Sticker != nil:
		b.app.OnSticker(ctx, b, (*telegram.Message)(m))
	case m.Video != nil:
		b.app.OnVideo(ctx, b, (*telegram.Message)(m))
	case m.VideoNote != nil:
		b.app.OnVideoNote(ctx, b, (*telegram.Message)(m))
	case m.Contact != nil:
		b.app.OnContact(ctx, b, (*telegram.Message)(m))
	case m.Location != nil:
		b.app.OnLocation(ctx, b, (*telegram.Message)(m))
	case m.Venue != nil:
		b.app.OnVenue(ctx, b, (*telegram.Message)(m))
	case m.Dice != nil:
		b.app.OnDice(ctx, b, (*telegram.Message)(m))
	default:
		return false
	}
	return true
}

func isUserInList(user *tb.User, list []tb.User) bool {
	for _, user2 := range list {
		if user.ID == user2.ID {
			return true
		}
	}
	return false
}

func embedSendOptions(params map[string]string, opt *tb.SendOptions) {
	if opt == nil {
		return
	}

	if opt.ReplyTo != nil && opt.ReplyTo.ID != 0 {
		params["reply_to_message_id"] = strconv.Itoa(opt.ReplyTo.ID)
	}

	if opt.DisableWebPagePreview {
		params["disable_web_page_preview"] = "true"
	}

	if opt.DisableNotification {
		params["disable_notification"] = "true"
	}

	if opt.ParseMode != tb.ModeDefault {
		params["parse_mode"] = opt.ParseMode
	}

	if opt.ReplyMarkup != nil {
		processButtons(opt.ReplyMarkup.InlineKeyboard)
		replyMarkup, _ := json.Marshal(opt.ReplyMarkup)
		params["reply_markup"] = string(replyMarkup)
	}
}

func processButtons(keys [][]tb.InlineButton) {
	if keys == nil || len(keys) < 1 || len(keys[0]) < 1 {
		return
	}

	for i := range keys {
		for j := range keys[i] {
			key := &keys[i][j]
			if key.Unique != "" {
				// Format: "\f<callback_name>|<data>"
				data := key.Data
				if data == "" {
					key.Data = "\f" + key.Unique
				} else {
					key.Data = "\f" + key.Unique + "|" + data
				}
			}
		}
	}
}

func copyReplyMarkup(r *tb.ReplyMarkup) *tb.ReplyMarkup {
	cp := *r

	cp.ReplyKeyboard = make([][]tb.ReplyButton, len(r.ReplyKeyboard))
	for i, row := range r.ReplyKeyboard {
		cp.ReplyKeyboard[i] = make([]tb.ReplyButton, len(row))
		copy(cp.ReplyKeyboard[i], row)
	}

	cp.InlineKeyboard = make([][]tb.InlineButton, len(r.InlineKeyboard))
	for i, row := range r.InlineKeyboard {
		cp.InlineKeyboard[i] = make([]tb.InlineButton, len(row))
		copy(cp.InlineKeyboard[i], row)
	}

	return &cp
}

func copySendOptions(og *tb.SendOptions) *tb.SendOptions {
	cp := *og
	if cp.ReplyMarkup != nil {
		cp.ReplyMarkup = copyReplyMarkup(cp.ReplyMarkup)
	}
	return &cp
}

func extractOptions(how []interface{}) *tb.SendOptions {
	var opts *tb.SendOptions

	for _, prop := range how {
		switch opt := prop.(type) {
		case *tb.SendOptions:
			opts = copySendOptions(opt)
		case *tb.ReplyMarkup:
			if opts == nil {
				opts = &tb.SendOptions{}
			}
			opts.ReplyMarkup = copyReplyMarkup(opt)
		case tb.Option:
			if opts == nil {
				opts = &tb.SendOptions{}
			}

			switch opt {
			case tb.NoPreview:
				opts.DisableWebPagePreview = true
			case tb.Silent:
				opts.DisableNotification = true
			case tb.ForceReply:
				if opts.ReplyMarkup == nil {
					opts.ReplyMarkup = &tb.ReplyMarkup{}
				}
				opts.ReplyMarkup.ForceReply = true
			case tb.OneTimeKeyboard:
				if opts.ReplyMarkup == nil {
					opts.ReplyMarkup = &tb.ReplyMarkup{}
				}
				opts.ReplyMarkup.OneTimeKeyboard = true
			default:
				// TODO: log
			}
		case tb.ParseMode:
			if opts == nil {
				opts = &tb.SendOptions{}
			}
			opts.ParseMode = opt
		default:
			// TODO: log
		}
	}

	return opts
}
