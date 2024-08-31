package telegram

import (
	tb "gopkg.in/tucnak/telebot.v2"
)

type (
	Message            tb.Message
	Callback           tb.Callback
	Query              tb.Query
	ChosenInlineResult tb.ChosenInlineResult
	ShippingQuery      tb.ShippingQuery
	PreCheckoutQuery   tb.PreCheckoutQuery
	Poll               tb.Poll
	PollAnswer         tb.PollAnswer
	Location           tb.Location
	Command            tb.Command
	CallbackResponse   tb.CallbackResponse
)
