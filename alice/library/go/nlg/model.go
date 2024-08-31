package libnlg

import (
	"context"
	"encoding/json"
	"math/rand"

	"a.yandex-team.ru/alice/library/go/random"
)

type Asset struct {
	text     string
	speech   string
	textOnly bool
}

func (a *Asset) UnmarshalJSON(bytes []byte) error {
	rawJSON := struct {
		Text     string `json:"text"`
		Speech   string `json:"speech"`
		TextOnly bool   `json:"text_only"`
	}{}
	if err := json.Unmarshal(bytes, &rawJSON); err != nil {
		return err
	}
	a.text = rawJSON.Text
	a.speech = rawJSON.Speech
	a.textOnly = rawJSON.TextOnly
	return nil
}

func (a *Asset) MarshalJSON() ([]byte, error) {
	return json.Marshal(struct {
		Text     string `json:"text"`
		Speech   string `json:"speech"`
		TextOnly bool   `json:"text_only"`
	}{
		Text:     a.text,
		Speech:   a.speech,
		TextOnly: a.textOnly,
	})
}

func NewAsset(text, speech string) Asset {
	return Asset{
		text:   text,
		speech: speech,
	}
}

func NewAssetWithText(text string) Asset {
	return Asset{
		text: text,
	}
}

func (a Asset) UseTextOnly(textOnly bool) Asset {
	return Asset{
		text:     a.text,
		speech:   a.speech,
		textOnly: textOnly,
	}
}

func (a Asset) Text() string {
	return a.text
}

func (a Asset) Speech() string {
	if a.textOnly {
		return ""
	}
	if len(a.speech) > 0 {
		return a.speech
	}
	return a.text
}

func (a Asset) IsTextOnly() bool {
	return a.textOnly
}

type NLG []Asset

var SilentResponse NLG = nil

func FromVariant(variant string) NLG {
	return NLG{NewAsset(variant, variant)}
}

func FromVariants(variants []string) NLG {
	nlg := make(NLG, 0, len(variants))
	for _, variant := range variants {
		nlg = append(nlg, NewAsset(variant, variant))
	}
	return nlg
}

func (nlg NLG) RandomAsset(ctx context.Context) Asset {
	ctxRand, err := random.RandFromContext(ctx)
	if err != nil {
		return nlg[rand.Intn(len(nlg))]
	}
	return nlg[ctxRand.Intn(len(nlg))]
}

func (nlg NLG) Clone() NLG {
	copied := make(NLG, len(nlg))
	copy(copied, nlg)
	return copied
}

func (nlg NLG) UseTextOnly(textOnly bool) NLG {
	cloned := nlg.Clone()
	for i := range cloned {
		cloned[i] = cloned[i].UseTextOnly(textOnly)
	}
	return cloned
}
