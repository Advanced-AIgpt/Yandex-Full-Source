package useragent

import (
	"regexp"
)

var (
	IOTAppRe       = regexp.MustCompile(`(^|\s)IOT/(Test/)?\d+\.\d+(\s|$)`)
	CentaurRe      = regexp.MustCompile(`(^|\s)centaur/(\S+)(\s|$)`)
	SearchPortalRe = regexp.MustCompile(`YaApp_(iOS|Android)/`)
)
