package api

import (
	"encoding/json"
	"fmt"
	"net/url"
	"strings"
)

type Artist struct {
	Name string `json:"name"`
}

type Track struct {
	ID         int         `json:"id"`
	Artists    []Artist    `json:"artists"`
	Available  bool        `json:"available"`
	Explicit   bool        `json:"explicit"`
	Title      string      `json:"title"`
	PoetryCite *PoetryCite `json:"poetryCite"`
}

type PoetryCite struct {
	Lines []PoetryLine `json:"lines"`
}

func (cite *PoetryCite) ToLyricsSnippet(snippetLength int) string {
	var builder strings.Builder
	for i, line := range cite.Lines {
		if i >= snippetLength {
			break
		}
		if i != 0 {
			builder.WriteRune('\n')
		}
		builder.WriteString(line.Text)
	}
	return builder.String()
}

type PoetryLine struct {
	Text string `json:"text"`
	Line int    `json:"line"`
}

func (track *Track) ToSearchResult(snippetLength int) SearchResult {
	artistNames := make([]string, len(track.Artists))
	for i, artist := range track.Artists {
		artistNames[i] = artist.Name
	}
	trackURL := url.URL{
		Scheme: "https",
		Host:   "music.yandex.ru",
		Path:   fmt.Sprintf("/track/%d", track.ID),
	}
	text := ""
	if track.PoetryCite != nil {
		text = track.PoetryCite.ToLyricsSnippet(snippetLength)
	}
	return SearchResult{
		Artist: strings.Join(artistNames, ", "),
		Title:  track.Title,
		Text:   text,
		URL:    trackURL.String(),

		Explicit: track.Explicit,
	}
}

type TracksInfo struct {
	Total  int     `json:"total"`
	Tracks []Track `json:"results"`
}

type MusicSearchResponseBody struct {
	Text       string     `json:"text"`
	TracksInfo TracksInfo `json:"tracks"`
}

type MusicSearchErrorBody struct {
	Message string `json:"message"`
	Name    string `json:"name"`
}

type MusicSearchResponse struct {
	InvocationInfo json.RawMessage          `json:"invocationInfo"`
	ResponseBody   *MusicSearchResponseBody `json:"result"`
	ErrorBody      *MusicSearchErrorBody    `json:"error"`
}
