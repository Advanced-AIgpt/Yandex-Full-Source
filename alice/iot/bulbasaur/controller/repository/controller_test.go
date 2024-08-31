package repository

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/atomic"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/metrics/mock"
)

func TestRepositoryController(t *testing.T) {
	suite.Run(t, &RepositorySuite{})
}

func (suite *RepositorySuite) TestSelectUserInfoWithPumpkin() {
	suite.Run("userinfo_from_pumpkin_because_of_timeout", func() {
		testRepo := suite.newTestRepository()
		testRepo.repo.signals.ydbPumpkinCacheHit = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheNotUsed = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheTotal = &mock.Counter{Value: atomic.NewInt64(0)}
		user := model.User{ID: 100500}
		userInfo := model.UserInfo{}

		//fill cache
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return userInfo, nil
			},
		}
		_, err := testRepo.repo.UserInfoWithPumpkin(testRepo.ctx, user)
		suite.Require().NoError(err)
		suite.Assert().Equal(testRepo.repo.YdbPumpkinCache.ItemCount(), 1)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)

		//check increased sensor with DeadlineExceeded error
		blockDBFunc := make(chan struct{})
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				// don't return from database until get result in main code
				<-blockDBFunc
				return model.UserInfo{}, context.DeadlineExceeded
			},
		}
		_, err = testRepo.repo.userInfoWithPumpkinTimeout(testRepo.ctx, user, time.Second*0)

		close(blockDBFunc)

		suite.Assert().NoError(err)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(2), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)
	})
	suite.Run("userinfo_from_pumpkin_because_of_ydb_error", func() {
		testRepo := suite.newTestRepository()
		testRepo.repo.signals.ydbPumpkinCacheHit = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheNotUsed = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheTotal = &mock.Counter{Value: atomic.NewInt64(0)}
		user := model.User{ID: 100500}
		userInfo := model.UserInfo{}

		//fill cache
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return userInfo, nil
			},
		}
		_, err := testRepo.repo.UserInfoWithPumpkin(testRepo.ctx, user)
		suite.Require().NoError(err)
		suite.Assert().Equal(testRepo.repo.YdbPumpkinCache.ItemCount(), 1)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)

		//check increased sensor with ydb.StatusUnavailable error
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return model.UserInfo{}, &ydb.OpError{Reason: ydb.StatusUnavailable}
			},
			IsDatabaseErrorMock: func(err error) bool {
				return true
			},
		}

		// database error initiates cache search and userInfo is found, because previous request finished successfully
		// no error is expected, cache hit counter should increase
		_, err = testRepo.repo.userInfoWithPumpkinTimeout(testRepo.ctx, user, time.Hour*24)

		suite.Assert().NoError(err)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(2), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)
	})
	suite.Run("userinfo_from_pumpkin_cache_miss", func() {
		testRepo := suite.newTestRepository()
		testRepo.repo.signals.ydbPumpkinCacheHit = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheTotal = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheNotUsed = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheMiss = &mock.Counter{Value: atomic.NewInt64(0)}
		user := model.User{ID: 100500}
		userInfo := model.UserInfo{}

		//fill cache
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return userInfo, nil
			},
		}
		_, err := testRepo.repo.UserInfoWithPumpkin(testRepo.ctx, user)
		suite.Require().NoError(err)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)

		//check sensor with ydb.StatusUnavailable error
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return model.UserInfo{}, &ydb.OpError{Reason: ydb.StatusUnavailable}
			},
			IsDatabaseErrorMock: func(err error) bool {
				return true
			},
		}

		//invalidate cache
		ydbPumpkinCacheKey := fmt.Sprintf("userinfo:%d", user.ID)
		testRepo.repo.YdbPumpkinCache.Delete(ydbPumpkinCacheKey)

		// database error initiates cache search and userInfo is not found in cache
		// no error is expected, cache miss counter should increase
		_, err = testRepo.repo.userInfoWithPumpkinTimeout(testRepo.ctx, user, time.Second*5)
		suite.Assert().NoError(err)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheMiss.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(2), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)
	})
	suite.Run("userinfo_from_pumpkin_cache_ignored", func() {
		testRepo := suite.newTestRepository()
		testRepo.repo.IgnoreCache = true
		testRepo.repo.signals.ydbPumpkinCacheHit = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheTotal = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheNotUsed = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheIgnored = &mock.Counter{Value: atomic.NewInt64(0)}
		testRepo.repo.signals.ydbPumpkinCacheMiss = &mock.Counter{Value: atomic.NewInt64(0)}
		user := model.User{ID: 100500}
		userInfo := model.UserInfo{}

		// normal requests work as expected, but cache is not set as a result
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return userInfo, nil
			},
		}
		_, err := testRepo.repo.UserInfoWithPumpkin(testRepo.ctx, user)
		suite.Require().NoError(err)
		suite.Assert().Equal(testRepo.repo.YdbPumpkinCache.ItemCount(), 0)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)

		//check sensor with ydb.StatusUnavailable error
		testRepo.repo.Database = &db.DBClientMock{
			SelectUserInfoMock: func(context.Context, uint64) (model.UserInfo, error) {
				return model.UserInfo{}, &ydb.OpError{Reason: ydb.StatusUnavailable}
			},
			IsDatabaseErrorMock: func(err error) bool {
				return true
			},
		}

		// database error initiates cache search, but cache is explicitly ignored
		// no error is expected, cache ignore counter should increase
		_, err = testRepo.repo.userInfoWithPumpkinTimeout(testRepo.ctx, user, time.Second*5)
		suite.Assert().NoError(err)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheHit.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(0), testRepo.repo.signals.ydbPumpkinCacheMiss.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheIgnored.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(1), testRepo.repo.signals.ydbPumpkinCacheNotUsed.(*mock.Counter).Value)
		suite.Assert().Equal(atomic.NewInt64(2), testRepo.repo.signals.ydbPumpkinCacheTotal.(*mock.Counter).Value)
	})

}
