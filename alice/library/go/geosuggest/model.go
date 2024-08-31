package geosuggest

import (
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type Option func(*Client) error

type GeosuggestFromAddressResponse struct {
	Part    string       `json:"part"` // text request, that was sent to geosuggest
	Results []Geosuggest `json:"results"`
}

func (g GeosuggestFromAddressResponse) HasValidAddress(address string, additionalComponents []string) (Geosuggest, bool) {
	for _, result := range g.Results {
		if result.ValidateAddress(address, additionalComponents) {
			return result, true
		}
	}
	return Geosuggest{}, false
}

type Geosuggest struct {
	RawCoordinates string          `json:"pos"` // format example: 37.559948,55.403053 (longitude,latitude)
	RawAddress     string          `json:"text"`
	Title          GeosuggestTitle `json:"title"`
}

type GeosuggestTitle struct {
	RawShortAddress string `json:"text"`
}

func (g Geosuggest) ValidateAddress(address string, additionalComponents []string) bool {
	// IOT-924: some datasync suggests contain old fashioned style of suggests
	// example: "Улица Льва Толстого, 16, Москва, Россия" instead of "Россия, Москва, улица Льва Толстого, 16"
	// additional components needed for mixing the substrings with input
	// for example: to validate "Московская область, Балашиха, квартал Абрамцево, 53" by "Россия, Московская область, Балашиха, квартал Абрамцево, 53" need to add "Россия"
	geosuggestComponents := strings.Split(g.Address(), ",")
	inputAddressComponents := strings.Split(address, ",")
	for i := range geosuggestComponents {
		geosuggestComponents[i] = tools.Standardize(geosuggestComponents[i])
	}
	for i := range inputAddressComponents {
		inputAddressComponents[i] = tools.Standardize(inputAddressComponents[i])
	}
	for i := range additionalComponents {
		additionalComponents[i] = tools.Standardize(additionalComponents[i])
	}
	inputAndAdditionalComponents := append(inputAddressComponents, additionalComponents...)

	return (slices.ContainsAll(inputAddressComponents, geosuggestComponents) || slices.ContainsAll(inputAndAdditionalComponents, geosuggestComponents)) &&
		slices.ContainsAll(geosuggestComponents, inputAddressComponents)
}

func (g Geosuggest) Coordinates() (Coordinates, error) {
	coordsSplitted := strings.Split(g.RawCoordinates, ",")
	if len(coordsSplitted) != 2 {
		return Coordinates{}, xerrors.Errorf("failed to parse raw location")
	}
	longitude, err := strconv.ParseFloat(coordsSplitted[0], 64)
	if err != nil {
		return Coordinates{}, err
	}
	latitude, err := strconv.ParseFloat(coordsSplitted[1], 64)
	if err != nil {
		return Coordinates{}, err
	}
	return Coordinates{Longitude: longitude, Latitude: latitude}, nil
}

func (g Geosuggest) Address() string {
	return tools.StandardizeSpaces(g.RawAddress)
}

func (g Geosuggest) ShortAddress() string {
	return tools.Capitalize(tools.StandardizeSpaces(g.Title.RawShortAddress))
}

type Coordinates struct {
	Longitude float64
	Latitude  float64
}
