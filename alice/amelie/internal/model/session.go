package model

import (
	"errors"
	"time"
)

type (
	Session struct {
		ID             string    `json:"session_id" bson:"session_id"`
		UUID           string    `json:"uuid" bson:"uuid"`
		User           User      `json:"user" bson:"user"`
		State          State     `json:"state" bson:"state"`
		Rate           Rate      `json:"rate" bson:"rate"`
		LastUpdateTime time.Time `json:"last_update_time" bson:"last_update_time"`
	}
	User struct {
		Username              string     `json:"username" bson:"username"`
		Yandex                Yandex     `json:"yandex" bson:"yandex"`
		Accounts              []Account  `json:"accounts" bson:"accounts"`
		LastGreetingsShowTime time.Time  `json:"last_greetings_show_time" bson:"last_greetings_show_time"`
		Shortcuts             []Shortcut `json:"shortcuts" bson:"shortcuts"`
	}
	Shortcut struct {
		Text string `json:"text" bson:"text"`
	}
	Account struct {
		ID       string `json:"id" bson:"id"`
		Username string `json:"username" bson:"username"`
		DeviceID string `json:"device_id" bson:"device_id"`

		Token             string    `json:"-" bson:"token"`
		RefreshToken      string    `json:"-" bson:"refresh_token"`
		TokenDeadlineTime time.Time `json:"token_deadline_time" bson:"token_deadline_time"`
	}
	Yandex struct {
		IsInternal       bool      `json:"is_internal" bson:"is_internal"`
		VerificationTime time.Time `json:"verification_time" bson:"verification_time"`
	}
	State         string
	RateFlashback struct {
		Time   time.Time `json:"time" bson:"time"`
		ID     string    `json:"id" bson:"id"`
		Actual bool      `json:"actual" bson:"-"`
	}
	Rate struct {
		RPSHistory []RateFlashback `json:"rps_history" bson:"rps_history"`
	}
)

func (u *User) GetActiveAccount() (Account, error) {
	if len(u.Accounts) > 0 {
		return u.Accounts[0], nil
	}
	return Account{}, errors.New("active account isn't set")
}

func (s State) Empty() bool {
	return len(s) == 0
}
