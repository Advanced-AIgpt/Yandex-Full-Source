package hash

import (
	"crypto/md5"
	"fmt"
)

func MD5(s string) string {
	return MD5Bytes([]byte(s))
}

func MD5Bytes(bytes []byte) string {
	return fmt.Sprintf("%x", md5.Sum(bytes))
}
