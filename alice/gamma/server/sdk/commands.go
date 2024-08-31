package sdk

import (
	"context"
	"time"

	structpb "github.com/golang/protobuf/ptypes/struct"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/alice/gamma/server/sdk/matcher"
)

var InvalidCachedValueType = xerrors.New("invalid cached value type")
var InvalidValueType = xerrors.New("invalid value type")

func (server *Server) getMatcher(ctx context.Context, pattern *api.MatchRequest_Pattern) (matcher_ *matcher.Matcher, err error) {
	if cachedValue, ok := server.compiledPatternsCache.Get(pattern.Pattern); !ok {
		log.Debugf("Pattern '%s' not in cache, compiling", pattern.Pattern)
		compileTime := time.Now()
		if matcher_, err = matcher.Compile(pattern.Pattern); err != nil {
			log.Errorf("Compilation error on pattern '%s': %+v", pattern.Pattern, err)
			return nil, err
		}
		log.Infof("Compilation of '%s' took: %s", pattern.Pattern, time.Since(compileTime))
		server.compiledPatternsCache.Add(pattern.Pattern, matcher_)
	} else {
		if matcher_, ok = cachedValue.(*matcher.Matcher); !ok {
			return nil, InvalidCachedValueType
		}
		log.Debugf("Pattern '%s' is in cache, skipping compilation", pattern.Pattern)
	}
	return matcher_, nil
}

func (server *Server) Match(ctx context.Context, request *api.MatchRequest) (*api.MatchResponse, error) {
	matchTime := time.Now()
	entities := make(map[string][]matcher.Entity)
	for i, entity := range request.Entities {
		var value interface{}
		if request.Entities[i].ProtoValue != nil {
			value = entity.ProtoValue
		} else {
			value = entity.Value
		}
		entities[entity.Type] = append(
			entities[entity.Type],
			matcher.Entity{
				Start: int(entity.Start),
				End:   int(entity.End),
				Type:  entity.Type,
				Value: value,
			},
		)
	}
	log.Infof("SDK Match request entities: %+v", entities)
	response := &api.MatchResponse{}
	for _, pattern := range request.Patterns {
		log.Infof("SDK Match request (input:%s, pattern:['%s':'%s'], entities: %+v)",
			request.Input, pattern.Name, pattern.Pattern, entities)
		matcher_, err := server.getMatcher(ctx, pattern)
		if err != nil {
			return nil, err
		}
		if match := matcher_.Match(request.Input, entities); match != nil {
			log.Infof("Matched pattern: (input:%s, pattern:['%s':'%s'], entities: %+v)",
				request.Input, pattern.Name, pattern.Pattern, entities)
			matchProto := &api.MatchResponse_Match{
				Name:      pattern.Name,
				Variables: make(map[string]*api.MatchResponse_Values),
			}
			for name, values := range match.Variables {
				protoValues := &api.MatchResponse_Values{}
				for _, value := range values {
					switch value := value.(type) {
					case string:
						protoValues.ProtoValues = append(protoValues.ProtoValues, convertToProto(value))
					case []byte:
						protoValues.Values = append(protoValues.Values, value)
					case *structpb.Value:
						protoValues.ProtoValues = append(protoValues.ProtoValues, convertToProto(value))
					default:
						log.Errorf("Unknown type error on value : %+v", value)
						return nil, InvalidValueType
					}
				}
				matchProto.Variables[name] = protoValues
			}
			response.Matches = append(response.Matches, matchProto)
		}
	}
	log.Infof("SDK Call Match took %s", time.Since(matchTime))
	return response, nil
}

func convertToProto(value matcher.EntityValue) *structpb.Value {
	switch v := value.(type) {
	case string:
		return &structpb.Value{
			Kind: &structpb.Value_StringValue{
				StringValue: v,
			},
		}
	case *structpb.Value:
		return v
	default:
		return nil
	}
}
