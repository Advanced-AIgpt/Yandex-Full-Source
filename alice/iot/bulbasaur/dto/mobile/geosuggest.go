package mobile

import (
	"a.yandex-team.ru/alice/library/go/geosuggest"
)

type GeosuggestRequest struct {
	Address string `json:"address"`
}

type GeosuggestsView struct {
	Status    string           `json:"status"`
	RequestID string           `json:"request_id"`
	Suggests  []GeosuggestView `json:"suggests"`
}

func (gr *GeosuggestsView) FromGeosuggestResponse(resp geosuggest.GeosuggestFromAddressResponse) {
	gr.Suggests = make([]GeosuggestView, 0, len(resp.Results))
	for _, suggest := range resp.Results {
		var view GeosuggestView
		view.FromGeosuggest(suggest)
		gr.Suggests = append(gr.Suggests, view)
	}
}

type GeosuggestView struct {
	Address      string `json:"address"`
	ShortAddress string `json:"short_address"`
}

func (gv *GeosuggestView) FromGeosuggest(suggest geosuggest.Geosuggest) {
	gv.Address = suggest.Address()
	gv.ShortAddress = suggest.ShortAddress()
}
