package telegram

type EventType string

const (
	TextEvent               EventType = "text"
	CallbackEvent           EventType = "callback"
	VoiceEvent              EventType = "voice"
	LocationEvent           EventType = "location"
	PhotoEvent              EventType = "photo"
	DocumentEvent           EventType = "document"
	QueryEvent              EventType = "query"
	PinnedEvent             EventType = "pinned"
	AudioEvent              EventType = "audio"
	AnimationEvent          EventType = "animation"
	StickerEvent            EventType = "sticker"
	VideoEvent              EventType = "video"
	VideoNoteEvent          EventType = "video_note"
	ContactEvent            EventType = "contact"
	VenueEvent              EventType = "venue"
	DiceEvent               EventType = "dice"
	InvoiceEvent            EventType = "invoice"
	PaymentEvent            EventType = "payment"
	AddedToGroupEvent       EventType = "added_to_group"
	UserJoinedEvent         EventType = "user_joined"
	UserLeftEvent           EventType = "user_left"
	NewGroupTitleEvent      EventType = "new_group_title"
	NewGroupPhotoEvent      EventType = "new_group_photo"
	GroupPhotoDeletedEvent  EventType = "group_photo_deleted"
	EditedEvent             EventType = "edited"
	ChannelPostEvent        EventType = "channel_post"
	EditedChannelPostEvent  EventType = "edited_channel_post"
	ChosenInlineResultEvent EventType = "chosen_inline_result"
	ShippingEvent           EventType = "shipping"
	CheckoutEvent           EventType = "checkout"
	PollEvent               EventType = "poll"
	PollAnswerEvent         EventType = "poll_answer"
)