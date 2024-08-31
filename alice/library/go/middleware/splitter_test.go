package middleware

import (
	"github.com/stretchr/testify/assert"
	"math/rand"
	"net/http"
	"net/http/httptest"
	"testing"
)

type nopHandler struct {
}

func (n nopHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {}

func TestSplitRequestMiddleware(t *testing.T) {
	label := PartitionLabel("logbroker")

	testCases := []struct {
		name          string
		percent       int
		enabled       bool
		expectedCalls []PartitionLabel
	}{
		{
			enabled: true,
			name:    "100%",
			percent: 100,
			expectedCalls: []PartitionLabel{
				label,
				label,
				label,
				label,
				label,
				label,
			},
		},
		{
			enabled: true,
			name:    "0%",
			percent: 0,
			expectedCalls: []PartitionLabel{
				"",
				"",
				"",
			},
		},
		{
			enabled: true,
			name:    "50%",
			percent: 50,
			expectedCalls: []PartitionLabel{
				"",
				label,
				"",
				label,
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			rand.Seed(0) // seed if fixed
			splitMiddleware := SplitRequestsMiddleware(SplitPartitionConfig{
				Enabled: tc.enabled,
				Percent: tc.percent,
				Label:   label,
			})

			handler := splitMiddleware(nopHandler{})
			for _, expectedLabel := range tc.expectedCalls {
				r := httptest.NewRequest("GET", "http://sometest", nil)
				handler.ServeHTTP(httptest.NewRecorder(), r)
				assert.Equal(t, expectedLabel, GetRequestPartitionLabel(r.Context()))
			}
		})
	}
}
