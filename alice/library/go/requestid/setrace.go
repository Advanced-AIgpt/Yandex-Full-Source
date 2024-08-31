package requestid

import (
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	XRTLogToken          = "X-RTLog-Token"
	XAppHostRequestReqID = "X-AppHost-Request-Reqid"
	XAppHostRequestRUID  = "X-AppHost-Request-Ruid"
)

func ParseRTLogToken(token string) (timestamp int64, requestID string, activationID string, err error) {
	if token == "" {
		return 0, "", "", xerrors.New("token is empty")
	}
	tokenParts := strings.Split(token, "$")
	if len(tokenParts) != 3 {
		return 0, "", "", xerrors.Errorf("invalid rtlog token `%s`: expected 3 parts, got %d", token, len(tokenParts))
	}
	rawTimestamp := tokenParts[0]
	timestamp, err = strconv.ParseInt(rawTimestamp, 10, 64)
	if err != nil {
		return 0, "", "", xerrors.Errorf("can't parse timestamp from token `%s`: %w", token, err)
	}
	requestID = tokenParts[1]
	activationID = tokenParts[2]
	return timestamp, requestID, activationID, nil
}

func ConstructRTLogToken(timestamp int64, requestID string, activationID string) string {
	return fmt.Sprintf("%d$%s$%s", timestamp, requestID, activationID)
}

func ParseRTLogTokenFromRequest(r *http.Request) (timestamp int64, requestID string, activationID string, err error) {
	// apphost way of sending token in several headers
	appHostRequestID := r.Header.Get(XAppHostRequestReqID)
	appHostRequestUID := r.Header.Get(XAppHostRequestRUID)

	if appHostRequestID != "" && appHostRequestUID != "" {
		return ParseRTLogToken(appHostRequestID + "-" + appHostRequestUID)
	}

	// old way of sending rtlog token
	rtlogToken := r.Header.Get(XRTLogToken)
	return ParseRTLogToken(rtlogToken)
}
