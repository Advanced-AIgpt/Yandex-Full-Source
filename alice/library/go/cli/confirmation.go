package cli

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"regexp"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/library/go/core/log"
)

var yes = []string{"y", "yes"}
var forceYesFlagKey = flag.Bool("force-yes", false, "--force-yes -- allows to run without tty")

func AskForConfirmation(s string, logger log.Logger) bool {
	reader := bufio.NewReader(os.Stdin)
	fmt.Printf("%s [y/N]: ", s)
	response, err := reader.ReadString('\n')
	if err != nil {
		logger.Error(err.Error())
		os.Exit(0)
	}

	response = strings.ToLower(strings.TrimSpace(response))
	return slices.Contains(yes, strings.ToLower(response))
}

func Ban(reg, stringToCheck, message string, logger log.Logger) {
	re := regexp.MustCompile(reg)
	if re.MatchString(stringToCheck) {
		logger.Warn(message)
		os.Exit(1)
	}
}

func IsForceYes() bool {
	return *forceYesFlagKey
}
