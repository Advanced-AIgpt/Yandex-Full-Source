package sdk

import (
	"context"
	"testing"

	lru "github.com/hashicorp/golang-lru"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/library/go/test/assertpb"
)

func TestMatch(t *testing.T) {
	ctx := context.Background()
	cache, err := lru.New(5)
	assert.NoError(t, err)
	server := &Server{
		compiledPatternsCache: cache,
	}

	request := api.MatchRequest{
		Input: "да это же коровка хотя может и котик",
		Patterns: []*api.MatchRequest_Pattern{
			{Name: "guess", Pattern: "* это * $animal *"},
			{Name: "maybe", Pattern: "* может [быть] * $animal *"},
		},
		Entities: []*api.Entity{
			{Start: 3, End: 4, Type: "animal", Value: []byte("cow")},
			{Start: 7, End: 8, Type: "animal", Value: []byte("cat")},
		},
	}

	response, err := server.Match(ctx, &request)
	require.NoError(t, err)
	assertpb.Equal(t, &api.MatchResponse{
		Matches: []*api.MatchResponse_Match{
			{
				Name: "guess",
				Variables: map[string]*api.MatchResponse_Values{
					"animal": {Values: [][]byte{[]byte("cow")}},
				},
			},
			{
				Name: "maybe",
				Variables: map[string]*api.MatchResponse_Values{
					"animal": {Values: [][]byte{[]byte("cat")}},
				},
			},
		},
	}, response)
	assert.Equal(t, 2, server.compiledPatternsCache.Len())
}

func TestMatchAny(t *testing.T) {
	ctx := context.Background()
	cache, err := lru.New(5)
	assert.NoError(t, err)
	server := &Server{
		compiledPatternsCache: cache,
	}

	request := api.MatchRequest{
		Input: "да это же коровка",
		Patterns: []*api.MatchRequest_Pattern{
			{Name: "guess", Pattern: "* это же $Any *"},
		},
		Entities: []*api.Entity{},
	}

	response, err := server.Match(ctx, &request)
	require.NoError(t, err)
	assert.Equal(t, "коровка", response.Matches[0].Variables["Any"].ProtoValues[0].GetStringValue())
}
