package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"

	"github.com/stretchr/testify/assert"
)

func (s *DBClientSuite) TestSelectAllExperiments() {
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");

            DELETE FROM ExperimentsUserGroup;
			UPSERT INTO ExperimentsUserGroup (group_id, user_ids) VALUES
              ("group_1", '[1,2,3]'),
              ("group_2", '[4,5,6]');

            DELETE FROM Experiments;
			UPSERT INTO Experiments (hname, name, is_enabled, user_ids, allow_staff, allow_all, group_ids) VALUES
              (1, "exp1", false, '[7]', true, false, NULL),
              (2, "exp2", true, '[8]', false, true, '["group_1"]');

`, s.dbClient.Prefix)

	err := s.dbClient.Write(context.Background(), query, nil)
	assert.NoError(s.T(), err)

	experimentsSlice, err := s.dbClient.SelectAllExperiments(context.Background())
	assert.NoError(s.T(), err)

	experimentsMap := experimentsSlice.ToMap()
	exp := experimentsMap["exp1"]
	assert.Equal(s.T(), experiments.Name("exp1"), exp.GetName())
	assert.False(s.T(), exp.IsEnabled())
	assert.True(s.T(), exp.IsAllowedForStaff())
	assert.False(s.T(), exp.IsAllowedForAll())
	assert.True(s.T(), exp.ContainsUserID(7))
	assert.False(s.T(), exp.ContainsUserID(1))

	exp = experimentsMap["exp2"]
	assert.Equal(s.T(), experiments.Name("exp2"), exp.GetName())
	assert.True(s.T(), exp.IsEnabled())
	assert.False(s.T(), exp.IsAllowedForStaff())
	assert.True(s.T(), exp.IsAllowedForAll())
	assert.True(s.T(), exp.ContainsUserID(8))
	assert.True(s.T(), exp.ContainsUserID(1))
	assert.False(s.T(), exp.ContainsUserID(4))
}
