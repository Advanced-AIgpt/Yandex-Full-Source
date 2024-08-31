package api

import (
	"encoding/json"

	"golang.org/x/xerrors"

	skillsAPI "a.yandex-team.ru/alice/gamma/sdk/api"
)

type CardHeader struct {
	Text string `json:"title"`
}

func cardHeaderFromProto(protoHeader *skillsAPI.CardHeader) (*CardHeader, error) {
	if protoHeader == nil {
		return nil, xerrors.New("empty card header proto")
	}
	return &CardHeader{Text: protoHeader.Text}, nil
}

type CardButton struct {
	Text    string          `json:"text"`
	Payload json.RawMessage `json:"payload"`
	URL     string          `json:"url"`
}

func cardButtonFromProto(protoButton *skillsAPI.CardButton) (button *CardButton, err error) {
	if protoButton == nil {
		return button, xerrors.New("empty button proto")
	}
	button = &CardButton{
		Text:    protoButton.Text,
		Payload: protoButton.Payload,
		URL:     protoButton.Url,
	}
	return button, nil
}

type CardItem struct {
	ImageID     string      `json:"image_id"`
	Title       string      `json:"title"`
	Description string      `json:"description"`
	Button      *CardButton `json:"card_button,omitempty"`
}

func cardItemFromProto(protoItem *skillsAPI.CardItem) (item CardItem, err error) {
	if protoItem == nil {
		return item, xerrors.New("empty card item proto")
	}
	item = CardItem{
		ImageID:     protoItem.ImageId,
		Title:       protoItem.Title,
		Description: protoItem.Description,
	}
	if protoItem.Button != nil {
		item.Button, err = cardButtonFromProto(protoItem.Button)
		if err != nil {
			return item, err
		}
	}
	return item, nil
}

func cardItemsFromProto(protoItems []*skillsAPI.CardItem) (items []CardItem, err error) {
	items = make([]CardItem, len(protoItems))
	for i, protoItem := range protoItems {
		items[i], err = cardItemFromProto(protoItem)
		if err != nil {
			return nil, err
		}
	}
	return items, nil
}

type CardFooter struct {
	Text   string      `json:"text"`
	Button *CardButton `json:"card_button,omitempty"`
}

func cardFooterFromProto(protoFooter *skillsAPI.CardFooter) (footer *CardFooter, err error) {
	if protoFooter == nil {
		return nil, xerrors.New("empty card footer proto")
	}
	footer = &CardFooter{
		Text: protoFooter.Text,
	}
	if protoFooter.Button != nil {
		footer.Button, err = cardButtonFromProto(protoFooter.Button)
		if err != nil {
			return footer, err
		}
	}
	return footer, nil
}

type Card struct {
	Type        string      `json:"type"`
	ImageID     string      `json:"image_id"`
	Title       string      `json:"title"`
	Description string      `json:"description"`
	Button      *CardButton `json:"card_button,omitempty"`
	Header      *CardHeader `json:"card_header,omitempty"`
	Footer      *CardFooter `json:"card_footer,omitempty"`
	Items       []CardItem  `json:"card_items,omitempty"`
}

func cardFromProto(protoCard *skillsAPI.Card) (card *Card, err error) {
	if protoCard == nil {
		return card, xerrors.New("empty card proto")
	}
	card = &Card{
		Type:        protoCard.Type,
		ImageID:     protoCard.ImageId,
		Title:       protoCard.Title,
		Description: protoCard.Description,
	}
	if protoCard.Button != nil {
		card.Button, err = cardButtonFromProto(protoCard.Button)
		if err != nil {
			return card, err
		}
	}
	if protoCard.Header != nil {
		card.Header, err = cardHeaderFromProto(protoCard.Header)
		if err != nil {
			return card, err
		}
	}
	if protoCard.Items != nil {
		card.Items, err = cardItemsFromProto(protoCard.Items)
		if err != nil {
			return card, err
		}
	}
	if protoCard.Footer != nil {
		card.Footer, err = cardFooterFromProto(protoCard.Footer)
		if err != nil {
			return card, err
		}
	}
	return card, nil
}
