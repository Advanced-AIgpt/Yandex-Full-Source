package scenario

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestScenarioController(t *testing.T) {
	suite.Run(t, &ScenariosSuite{})
}
