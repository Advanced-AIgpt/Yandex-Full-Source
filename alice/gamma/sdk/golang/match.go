package sdk

import (
	"encoding/json"
	"fmt"
	"regexp"
	"sort"
	"strings"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

type Pattern struct {
	Name    string
	Pattern string
}

func (pattern *Pattern) toProto() *api.MatchRequest_Pattern {
	return &api.MatchRequest_Pattern{
		Name:    pattern.Name,
		Pattern: pattern.Pattern,
	}
}

type Hypothesis struct {
	Name      string
	Variables map[string][]interface{}
}

func hypothesisFromProto(proto *api.MatchResponse_Match) (Hypothesis, error) {
	result := Hypothesis{
		Name:      proto.Name,
		Variables: make(map[string][]interface{}),
	}
	for name, values := range proto.Variables {
		if len(values.ProtoValues) > 0 {
			variables := make([]interface{}, len(values.ProtoValues))
			for i, value := range values.ProtoValues {
				variables[i] = value
			}
			result.Variables[name] = variables
		} else {
			variables := make([]interface{}, len(values.Values))
			for i, value := range values.Values {
				err := json.Unmarshal(value, &variables[i])
				if err != nil {
					return result, err
				}
			}
			result.Variables[name] = variables
		}
	}
	return result, nil
}

func (context *SkillContext) match(input string, patterns []Pattern, entities []Entity) ([]Hypothesis, error) {
	request := &api.MatchRequest{
		Input:    input,
		Patterns: make([]*api.MatchRequest_Pattern, len(patterns)),
		Entities: make([]*api.Entity, len(entities)),
	}
	context.connection.logger.Infof("Trying to match (input:%s; patterns:%+v; entities: %+v)", input, patterns, entities)

	for i, pattern := range patterns {
		request.Patterns[i] = pattern.toProto()
	}

	var err error
	for i, entity := range entities {
		request.Entities[i], err = entity.toProto()
		if err != nil {
			context.connection.logger.Errorf("Failed to serialize entity %v: %v", entity, err)
			return nil, err
		}
	}

	response, err := context.connection.client.Match(context.ctx, request)
	if err != nil {
		context.connection.logger.Errorf("Failed to match request %v: %v", request, err)
		return nil, err
	}
	result := make([]Hypothesis, len(response.Matches))

	for i, match := range response.Matches {
		result[i], err = hypothesisFromProto(match)
		if err != nil {
			context.connection.logger.Errorf("Failed to deserialize match %v: %v", match, err)
			return nil, err
		}
	}

	return result, nil
}

type Extractor interface {
	GetEntities(string) []Entity
}

type defaultExtractor struct {
}

func (*defaultExtractor) GetEntities(string) []Entity {
	return []Entity{}
}

var EmptyEntityExtractor = defaultExtractor{}

type EntityExtractor struct {
	entities map[string]*regexp.Regexp
}

func NewEntityExtractor(entities map[string]map[string][]string) *EntityExtractor {
	entityExtractor := &EntityExtractor{
		entities: make(map[string]*regexp.Regexp),
	}
	for entityType, entityObjects := range entities {
		synonyms := make([]string, 0)

		for entityTypeName, entityTokens := range entityObjects {
			synonyms = append(synonyms, fmt.Sprintf(`(?P<%s>`, entityTypeName)+strings.Join(entityTokens, "|")+`)`)
		}
		entityExtractor.entities[entityType] = regexp.MustCompile(`(?:[\pP\s]+|^)` + `(?:` + strings.Join(synonyms, "|") +
			`)(?:[\pP\s]+|$)`)
	}
	return entityExtractor
}

var notSpacesPunctuation = regexp.MustCompile(`[^\s\pP]+`)

func findAllTokenIndex(command string) (beginnings, endings []int) {
	if len(command) == 0 {
		return
	}

	wordsIndexes := notSpacesPunctuation.FindAllIndex([]byte(command), -1)
	beginnings = make([]int, len(wordsIndexes))
	endings = make([]int, len(wordsIndexes))
	for i := range wordsIndexes {
		beginnings[i] = wordsIndexes[i][0]
		endings[i] = wordsIndexes[i][1]
	}
	return
}

func (extractor *EntityExtractor) GetEntities(command string) []Entity {
	result := make([]Entity, 0)
	if len(command) == 0 {
		return result
	}
	beginnings, endings := findAllTokenIndex(command)

	for enType, re := range extractor.entities {
		entity := Entity{
			Type:  enType,
			Start: int64(-1),
		}
		for index, match := 0, re.FindStringSubmatchIndex(command); match != nil; match = re.FindStringSubmatchIndex(command[index:]) {
			for i := 1; i < len(match)/2; i++ {
				if match[i*2] == -1 || re.SubexpNames()[i] == "" {
					continue
				}
				start := index + match[i*2]
				end := index + match[i*2+1]
				entity.Value = re.SubexpNames()[i]
				entity.Start = int64(sort.SearchInts(beginnings, start))
				entity.End = int64(sort.SearchInts(endings, end) + 1)
				result = append(result, entity)
				index = end
				break
			}
		}
	}
	return result
}

func (context *SkillContext) Match(request *Request, patterns []Pattern, extractor Extractor) ([]Hypothesis, error) {
	entities := extractor.GetEntities(request.Command)
	if request.Nlu != nil {
		entities = append(entities, request.Nlu.Entities...)
	}

	return context.match(request.Command, patterns, entities)
}
