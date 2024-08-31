package executors

import (
	"github.com/stretchr/testify/assert"
	"testing"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestErrorCheck(t *testing.T) {
	testCases := []struct {
		name        string
		targetError error
		expected    bool
	}{
		{
			name:        "non token error",
			targetError: xerrors.New("some error"),
			expected:    false,
		},
		{
			name:        "token error",
			targetError: xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
			expected:    true,
		},
		{
			name: "no error in a list",
			targetError: bulbasaur.Errors{
				xerrors.New("some error"),
				xerrors.New("another error"),
				xerrors.New("different error"),
				xerrors.New("some another error"),
			},
			expected: false,
		},
		{
			name: "token error only one in a list",
			targetError: bulbasaur.Errors{
				xerrors.New("some error"),
				xerrors.New("another error"),
				xerrors.New("different error"),
				xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
				xerrors.New("some another error"),
			},
			expected: false,
		},
		{
			name: "token error are all in a list",
			targetError: bulbasaur.Errors{
				xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
				xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
				xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
				xerrors.Errorf("token 404 error: %w", &socialism.TokenNotFoundError{}),
			},
			expected: true,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, isTokenNotFoundError(tc.targetError))
		})
	}
}
