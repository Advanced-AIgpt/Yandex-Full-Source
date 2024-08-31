package telebot

import (
	"encoding/json"
	"fmt"
	"regexp"
	"strconv"

	tb "gopkg.in/tucnak/telebot.v2"
)

var errorRx = regexp.MustCompile(`{.+"error_code":(\d+),"description":"(.+)".*}`)

func extractOk(data []byte) error {
	match := errorRx.FindStringSubmatch(string(data))
	if match == nil || len(match) < 3 {
		return nil
	}

	desc := match[2]
	err := tb.ErrByDescription(desc)
	if err == nil {
		code, _ := strconv.Atoi(match[1])
		err = fmt.Errorf("telegram unknown: %s (%d)", desc, code)
	}
	return err
}

func extractMessage(data []byte) (*tb.Message, error) {
	var resp struct {
		Result *tb.Message
	}
	if err := json.Unmarshal(data, &resp); err != nil {
		return nil, fmt.Errorf("extract message error: %w", err)
	}
	return resp.Result, nil
}
