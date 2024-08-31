package tools

import (
	"net/url"
	"path"
	"strings"
)

func URLJoin(base string, parts ...string) string {
	u, _ := url.Parse(base)
	totalParts := append([]string{u.Path}, parts...)
	u.Path = path.Join(totalParts...)
	result := u.String()
	if strings.HasSuffix(parts[len(parts)-1], "/") {
		result += "/"
	}

	return result
}
