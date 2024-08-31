package sdk

import (
	"bytes"
	"encoding/json"
	"image"
	"image/png"
	"net/http"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

type CardHeader struct {
	Text string
}

func (header *CardHeader) toProto() *api.CardHeader {
	return &api.CardHeader{Text: header.Text}
}

type CardButton struct {
	Text    string
	Payload interface{}
	URL     string
}

func (button *CardButton) toProto() (apiButton *api.CardButton, err error) {
	apiButton = &api.CardButton{
		Text: button.Text,
		Url:  button.URL,
	}
	if button.Payload != nil {
		apiButton.Payload, err = json.Marshal(button.Payload)
		if err != nil {
			return nil, err
		}
	}
	return apiButton, nil
}

type CardItem struct {
	ImageID     string
	Title       string
	Description string
	Button      *CardButton
}

func (item *CardItem) toProto() (protoItem *api.CardItem, err error) {
	protoItem = &api.CardItem{
		ImageId:     item.ImageID,
		Title:       item.Title,
		Description: item.Description,
	}
	if item.Button != nil {
		protoItem.Button, err = item.Button.toProto()
		if err != nil {
			return nil, err
		}
	}
	return protoItem, nil
}

func cardItemsToProto(cardItems []CardItem) (protoItems []*api.CardItem, err error) {
	protoItems = make([]*api.CardItem, len(cardItems))
	for i, item := range cardItems {
		protoItems[i], err = item.toProto()
		if err != nil {
			return nil, err
		}
	}
	return protoItems, nil
}

type CardFooter struct {
	Text   string
	Button *CardButton
}

func (footer *CardFooter) toProto() (protoFooter *api.CardFooter, err error) {
	protoFooter = &api.CardFooter{Text: footer.Text}
	if footer.Button != nil {
		protoFooter.Button, err = footer.Button.toProto()
		if err != nil {
			return nil, err
		}
	}
	return protoFooter, nil
}

type Card struct {
	Type        string
	ImageID     string
	Title       string
	Description string
	Button      *CardButton
	Header      *CardHeader
	Items       []CardItem
	Footer      *CardFooter
}

func (card *Card) AddCardItems(items ...CardItem) {
	card.Items = append(card.Items, items...)
}

func (card *Card) toProto() (protoCard *api.Card, err error) {
	protoCard = &api.Card{
		Type:        card.Type,
		ImageId:     card.ImageID,
		Title:       card.Title,
		Description: card.Description,
	}
	if card.Header != nil {
		protoCard.Header = card.Header.toProto()
	}
	if card.Button != nil {
		protoCard.Button, err = card.Button.toProto()
		if err != nil {
			return nil, err
		}
	}
	if card.Items != nil {
		protoCard.Items, err = cardItemsToProto(card.Items)
		if err != nil {
			return nil, err
		}
	}
	if card.Footer != nil {
		protoCard.Footer, err = card.Footer.toProto()
		if err != nil {
			return nil, err
		}
	}
	return protoCard, nil
}

func PostImage(image image.Image, url string) (string, error) {
	buffer := new(bytes.Buffer)
	var err error

	if err = png.Encode(buffer, image); err != nil {
		return "", err
	}

	response, err := http.Post(url, "image/png", buffer)
	if err != nil {
		return "", err
	}

	var responseBody map[string]interface{}
	if err = json.NewDecoder(response.Body).Decode(&responseBody); err != nil {
		return "", err
	}

	return responseBody["url"].(string), nil
}
