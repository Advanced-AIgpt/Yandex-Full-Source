package sdk

import (
	"context"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

type sdkMatchingMock struct{}

func (*sdkMatchingMock) Match(context.Context, *api.MatchRequest) (*api.MatchResponse, error) {
	return &api.MatchResponse{
		Matches: []*api.MatchResponse_Match{
			{
				Name: "any",
				Variables: map[string]*api.MatchResponse_Values{
					"animal": {Values: [][]byte{[]byte("\"cat\"")}},
				},
			},
		},
	}, nil
}

func TestSdkMatch(t *testing.T) {
	addr := runTestServer(t, &sdkMatchingMock{})

	apiClient := client{apiAddr: addr}

	connection, err := apiClient.connect(zap.S())
	require.NoError(t, err)

	skillContext := SkillContext{ctx: context.Background(), connection: connection}
	matches, err := skillContext.Match(
		&Request{Command: "Алиса ты котик"},
		[]Pattern{
			{"any", "* ты $animal"},
			{"none", "нет ты"},
		},
		&EmptyEntityExtractor,
	)
	require.NoError(t, err)
	assert.Equal(t, []Hypothesis{
		{
			Name: "any",
			Variables: map[string][]interface{}{
				"animal": {"cat"},
			},
		},
	}, matches)
}

func TestSdkMatch_GetEntities(t *testing.T) {
	entityExtractor := NewEntityExtractor(map[string]map[string][]string{
		"Animal": {
			"cat": {
				"кошка", "кошки", "кошке", "кошкой", "кошку",
				"кошечка", "кошечки", "кошечке", "кошечкой", "кошечку",
				"кот", "кота", "коту", "коте", "котом",
				"котик", "котика", "котику", "котике", "котиком",
			},
			"dog": {
				"собака", "собаки", "собаку", "собаке", "собакой",
				"собачка", "собачки", "собачку", "собачке", "собачкой",
				"пес", "пса", "псу", "псе", "псом",
				"песик", "песика", "песику", "песике", "песиком",
			},
			"cow": {
				"корова", "коровы", "корове", "корову", "коровой",
				"коровка", "коровки", "коровке", "коровку", "коровкой",
			},
			"seacat": {
				"морской котик",
			},
		},
		"Words": {
			"hello": {"привет", "здравствуйте"},
			"maybe": {`может быть`, "возможно", "кажется", "наверное"},
		},
		"Actor": {
			"brad": {
				`бред пит`,
			},
		},
	})
	//1 test
	query := "\tможет быть, это котик с кошечкой\t"
	expected := []Entity{
		{
			Start: 0,
			End:   2,
			Value: "maybe",
			Type:  "Words",
		},
		{
			Start: 3,
			End:   4,
			Value: "cat",
			Type:  "Animal",
		},
		{
			Start: 5,
			End:   6,
			Value: "cat",
			Type:  "Animal",
		},
	}
	output := entityExtractor.GetEntities(query)
	sort.Slice(output, func(i, j int) bool {
		return output[i].Start < output[j].End
	})
	assert.Equal(t, expected, output)

	query = "котик с кошечкой и коровкой, наверное"
	expected = []Entity{
		{
			Start: 0,
			End:   1,
			Value: "cat",
			Type:  "Animal",
		},
		{
			Start: 2,
			End:   3,
			Value: "cat",
			Type:  "Animal",
		},
		{
			Start: 4,
			End:   5,
			Value: "cow",
			Type:  "Animal",
		},
		{
			Start: 5,
			End:   6,
			Value: "maybe",
			Type:  "Words",
		},
	}
	output = entityExtractor.GetEntities(query)
	sort.Slice(output, func(i, j int) bool {
		return output[i].Start < output[j].End
	})
	assert.Equal(t, expected, output)

	query = "       может быть, это     котик с кошечкой   "
	expected = []Entity{
		{
			Start: 0,
			End:   2,
			Value: "maybe",
			Type:  "Words",
		},
		{
			Start: 3,
			End:   4,
			Value: "cat",
			Type:  "Animal",
		},
		{
			Start: 5,
			End:   6,
			Value: "cat",
			Type:  "Animal",
		},
	}
	output = entityExtractor.GetEntities(query)
	sort.Slice(output, func(i, j int) bool {
		return output[i].Start < output[j].Start
	})
	assert.Equal(t, expected, output)

	query = "\t\t\t"
	output = entityExtractor.GetEntities(query)
	expected = []Entity{}
	assert.Equal(t, expected, output)

	query = "кошечка"
	output = entityExtractor.GetEntities(query)
	expected = []Entity{
		{
			Start: 0,
			End:   1,
			Value: "cat",
			Type:  "Animal",
		},
	}
	assert.Equal(t, expected, output)
	query = "       бред пит лялялляля"
	output = entityExtractor.GetEntities(query)
	expected = []Entity{
		{
			Start: 0,
			End:   2,
			Value: "brad",
			Type:  "Actor",
		},
	}
	assert.Equal(t, expected, output)

	query = "я морской котик или кошечка"
	output = entityExtractor.GetEntities(query)
	sort.Slice(output, func(i, j int) bool {
		return output[i].Start < output[j].Start
	})
	expected = []Entity{
		{
			Start: 1,
			End:   3,
			Value: "seacat",
			Type:  "Animal",
		},
		{
			Start: 4,
			End:   5,
			Value: "cat",
			Type:  "Animal",
		},
	}
	assert.Equal(t, expected, output)
	query = "я !!!морской котик!!! или кошечка"
	output = entityExtractor.GetEntities(query)

	sort.Slice(output, func(i, j int) bool {
		return output[i].Start < output[j].Start
	})
	expected = []Entity{
		{
			Start: 1,
			End:   3,
			Value: "seacat",
			Type:  "Animal",
		},
		{
			Start: 4,
			End:   5,
			Value: "cat",
			Type:  "Animal",
		},
	}
	assert.Equal(t, expected, output)

	query = "фубред питон"
	output = entityExtractor.GetEntities(query)
	expected = []Entity{}
	assert.Equal(t, expected, output)
}
