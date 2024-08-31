package dbexpmanager

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	btest "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	quasarblackbox "a.yandex-team.ru/alice/library/go/blackbox"
)

type DBMock struct {
	*db.DBClientMock
}

const (
	expA         = "disabled_Exp"
	expB         = "enabled_for_all"
	expC         = "enabled_for_staff"
	expD         = "enabled_for_common_user"
	commonUserID = 100500
	staffUserID  = 100501
)

func (d DBMock) SelectAllExperiments(context.Context) (experiments.Experiments, error) {
	return experiments.Experiments{
		*experiments.NewExperiment(expA, false, []uint64{}, false, true),
		*experiments.NewExperiment(expB, true, []uint64{}, false, true),
		*experiments.NewExperiment(expC, true, []uint64{}, true, false),
		*experiments.NewExperiment(expD, true, []uint64{commonUserID}, false, false),
	}, nil
}

func Test_ExperimentsCommon(t *testing.T) {
	logger := btest.NopLogger()
	f := NewManagerFactory(logger, &DBMock{}, &quasarblackbox.ClientMock{})
	err := f.Start()
	assert.NoError(t, err)
	defer f.Stop()

	m := f.NewManager()
	commonUser := model.User{
		ID:     commonUserID,
		Login:  "duhast_vyacheslavovich",
		Ticket: "",
	}
	m.SetStaff(commonUserID, false)
	inputs := []struct {
		Name     experiments.Name
		Expected bool
	}{
		{Name: expA, Expected: false},
		{Name: expB, Expected: true},
		{Name: expC, Expected: false},
		{Name: expD, Expected: true},
	}

	for _, input := range inputs {
		t.Run(input.Name.String(), func(t *testing.T) {
			assert.Equal(t, input.Expected, m.IsUserExperimentEnabled(context.Background(), input.Name, commonUser))
		})
	}
}

func Test_ExperimentsStaff(t *testing.T) {
	logger := btest.NopLogger()
	f := NewManagerFactory(logger, &DBMock{}, &quasarblackbox.ClientMock{})
	err := f.Start()
	assert.NoError(t, err)
	defer f.Stop()

	m := f.NewManager()
	staffUser := model.User{
		ID:     staffUserID,
		Login:  "duhast_vyacheslavovich",
		Ticket: "",
	}
	m.SetStaff(staffUserID, true)

	inputs := []struct {
		Name     experiments.Name
		Expected bool
	}{
		{Name: expA, Expected: false},
		{Name: expB, Expected: true},
		{Name: expC, Expected: true},
		{Name: expD, Expected: false},
	}

	for _, input := range inputs {
		t.Run(input.Name.String(), func(t *testing.T) {
			assert.Equal(t, input.Expected, m.IsUserExperimentEnabled(context.Background(), input.Name, staffUser))
		})
	}
}
