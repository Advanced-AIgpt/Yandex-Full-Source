package csrf

import (
	"crypto/hmac"
	"crypto/sha1"
	"encoding/hex"
	"fmt"
	"strconv"
	"strings"
	"time"
)

type CsrfTool struct {
	tokenKey string
}

func (c *CsrfTool) Init(tokenKey string) error {
	if len(tokenKey) == 0 {
		return fmt.Errorf("CSRF-key is empty")
	}
	c.tokenKey = tokenKey
	return nil
}

func (c *CsrfTool) CheckToken(tokenCandidate, yandexuid string, userID uint64) error {
	if len(tokenCandidate) == 0 {
		return fmt.Errorf("token_candidate is empty")
	}
	if len(yandexuid) == 0 {
		return fmt.Errorf("yandexuid is empty")
	}

	if !strings.Contains(tokenCandidate, ":") {
		return fmt.Errorf("wrong CSRF-token format")
	}

	splitted := strings.Split(tokenCandidate, ":")
	token := splitted[0]
	timestamp := splitted[1]
	timestampMs, err := strconv.Atoi(timestamp)
	if err != nil {
		return fmt.Errorf("invalid timestamp format: %s", timestamp)
	}

	if !c.checkTokenTimestamp(int32(timestampMs)) {
		return fmt.Errorf("invalid CSRF token timestamp: %s", timestamp)
	}

	generated := c.generateToken(timestamp, yandexuid, userID)
	if !hmac.Equal([]byte(token), []byte(generated)) {
		return fmt.Errorf("invalid CSRF token: %s, expected: %s", token, generated)
	}

	return nil
}

func (c *CsrfTool) checkTokenTimestamp(timestamp int32) bool {
	return abs(int32(time.Now().Unix())-timestamp) < 24*60*60
}

// According to https://wiki.yandex-team.ru/security/for/web-developers/csrf/
func (c *CsrfTool) generateToken(timestamp, yandexuid string, userID uint64) string {
	h := hmac.New(sha1.New, []byte(c.tokenKey))
	message := fmt.Sprintf("%s:%s:%s", strconv.FormatUint(userID, 10), yandexuid, timestamp)
	_, _ = h.Write([]byte(message))
	return hex.EncodeToString(h.Sum(nil))
}

func abs(x int32) int32 {
	if x < 0 {
		return -x
	}
	return x
}
