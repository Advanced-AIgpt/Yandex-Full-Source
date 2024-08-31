package common

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func UserInfoFromDataSource(ctx context.Context, dataSources map[int32]*scenarios.TDataSource) (model.UserInfo, error) {
	var userInfo model.UserInfo
	if iotDataSource, ok := dataSources[int32(common.EDataSourceType_IOT_USER_INFO)]; ok {
		userInfoProto := iotDataSource.GetIoTUserInfo()
		if userInfoProto == nil {
			datasourceErr := xerrors.New("datasource is marked as IOT_USER_INFO but does not hold data")
			return userInfo, datasourceErr
		}
		if err := userInfo.FromUserInfoProto(ctx, userInfoProto); err != nil {
			conversionErr := xerrors.Errorf("unable to convert data from datasource to UserInfo: %w", err)
			return userInfo, conversionErr
		}
		return userInfo, nil
	}

	return userInfo, xerrors.New("IOT_USER_INFO datasource not found")
}
