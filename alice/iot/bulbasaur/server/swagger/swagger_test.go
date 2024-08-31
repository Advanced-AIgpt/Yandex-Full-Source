package swagger

import (
	"bytes"
	"github.com/google/go-cmp/cmp"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"os"
	"os/exec"
	"testing"

	"a.yandex-team.ru/library/go/test/yatest"
)

func TestSwaggerSpec(t *testing.T) {
	swaggerPath, err := yatest.BinaryPath("vendor/github.com/go-swagger/go-swagger/cmd/swagger/swagger")
	require.NoError(t, err)
	resultPath := yatest.WorkPath("actual_swagger.json")
	cmd := exec.Command(swaggerPath, "generate", "spec", "-o", resultPath, "--scan-models")
	var (
		stdout bytes.Buffer
		stderr bytes.Buffer
	)
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	err = cmd.Run()
	assert.Equal(t, "", stderr.String())
	require.NoError(t, err)

	expectedSpec, err := os.ReadFile("swagger/swagger.json")
	require.NoError(t, err)

	actualSpec, err := os.ReadFile(resultPath)
	require.NoError(t, err)

	if !cmp.Equal(string(expectedSpec), string(actualSpec)) {
		assert.Fail(t, "please call 'go generate' for regenerating swagger spec "+cmp.Diff(string(expectedSpec), string(actualSpec)))
	}
}
