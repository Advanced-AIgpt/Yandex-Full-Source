package main

import (
	"math/rand"
	"time"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

type MusicAPIURLOption string

func (url *MusicAPIURLOption) Set(value string) (err error) {
	*url = MusicAPIURLOption(value)
	return nil
}

func (url *MusicAPIURLOption) String() string {
	return string(*url)
}

type MusicSkillOptions struct {
	MusicAPIURL string
}

func (options *MusicSkillOptions) GetOptions() []sdk.ValueOption {
	return []sdk.ValueOption{
		{Name: "music-api", Usage: "music api url", Value: (*MusicAPIURLOption)(&options.MusicAPIURL)},
	}
}
func main() {
	source := rand.NewSource(time.Now().Unix())
	random := rand.New(source)
	sdk.StartSkill(NewSongSearchSkill(*random), &MusicSkillOptions{"http://music-web-ext.music.yandex.net/internal-api/search"})
}
