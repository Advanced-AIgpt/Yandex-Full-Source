package api

import (
	"encoding/json"
	"net/http"
	"net/url"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

var URLEmptyError = xerrors.New("music api url empty")

type SearchResult struct {
	Artist string
	Title  string
	Text   string
	URL    string

	Explicit bool
}

type SongSearch interface {
	Search(line string) ([]SearchResult, error)
}

func formRequestURL(apiURL, text string) (string, error) {
	queryURL, err := url.Parse(apiURL)
	if err != nil {
		return "", err
	}
	queryValues := queryURL.Query()
	queryValues.Add("type", "lyrics")
	queryValues.Add("page", "0")
	queryValues.Add("text", text)
	queryURL.RawQuery = queryValues.Encode()
	return queryURL.String(), nil
}

type MusicSearchAPI struct {
	Logger sdk.Logger
	URL    string

	Limit         int
	SnippetLength int
}

func (api *MusicSearchAPI) getSearchResponse(query string) (response *MusicSearchResponse, _ error) {
	requestURL, err := formRequestURL(api.URL, query)
	if err != nil {
		return nil, err
	}
	api.Logger.Infof("Sending query: %s, request url: %s", query, requestURL)
	res, err := http.Get(requestURL)
	if err != nil {
		api.Logger.Errorf("Can't get %s - %+v", requestURL, err)
		return nil, err
	}

	if err := json.NewDecoder(res.Body).Decode(&response); err != nil {
		api.Logger.Errorf("Invalid response %+v", err)
		return nil, err
	}
	return response, nil
}

func min(left, right int) int {
	if left < right {
		return left
	}
	return right
}

func (api *MusicSearchAPI) parseSearchResponse(response *MusicSearchResponse) ([]SearchResult, error) {
	if response.ErrorBody != nil {
		errName := response.ErrorBody.Name
		errMessage := response.ErrorBody.Message
		requestInfo := string(response.InvocationInfo)
		api.Logger.Errorf("Music api error %s: %s; invocationInfo: %s", errName, errMessage, requestInfo)
		return nil, xerrors.Errorf("%s: %s", errName, errMessage)
	}
	tracksInfo := response.ResponseBody.TracksInfo
	searchResults := make([]SearchResult, min(api.Limit, tracksInfo.Total))
	for i := range searchResults {
		searchResults[i] = tracksInfo.Tracks[i].ToSearchResult(api.SnippetLength)
	}
	return searchResults, nil
}

func (api *MusicSearchAPI) Search(query string) ([]SearchResult, error) {
	response, err := api.getSearchResponse(query)
	if err != nil {
		return nil, err
	}
	searchResults, err := api.parseSearchResponse(response)
	if err != nil {
		return nil, err
	}
	return searchResults, nil
}
