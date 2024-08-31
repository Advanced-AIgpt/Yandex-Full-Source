package discovery

import "regexp"

var discoveryNameRegex = regexp.MustCompile(`^(([а-яёА-ЯЁ]+)|(\d+)|([a-zA-Z0-9]+))$`)

const (
	UpdatedDiffStatus DiffStatus = "UPDATED"
	NewDiffStatus     DiffStatus = "NEW"
	ZeroDiffStatus    DiffStatus = "ZERODIFF"
)
