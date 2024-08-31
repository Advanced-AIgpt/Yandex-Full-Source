package tools

import (
	"encoding/base64"
)

func Base64Encode(message []byte) []byte {
	b := make([]byte, base64.StdEncoding.EncodedLen(len(message)))
	base64.StdEncoding.Encode(b, message)
	return b
}

func Base64Decode(message []byte) ([]byte, error) {
	result := make([]byte, base64.StdEncoding.DecodedLen(len(message)))
	length, err := base64.StdEncoding.Decode(result, message)
	if err != nil {
		return nil, err
	}
	return result[:length], nil
}
