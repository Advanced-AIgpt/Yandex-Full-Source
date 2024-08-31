#include <alice/matrix/worker/library/services/worker/utils.h>

#include <library/cpp/protobuf/interop/cast.h>
#include <library/cpp/testing/gtest/gtest.h>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>


namespace {

NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy GetRetryPolicy(
    TDuration restartPeriodScale,
    ui64 restartPeriodBackOff,
    TDuration minRestartPeriod,
    TDuration maxRestartPeriod

) {
    NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy retryPolicy;

    retryPolicy.MutableRestartPeriodScale()->CopyFrom(
        NProtoInterop::CastToProto(restartPeriodScale)
    );
    retryPolicy.SetRestartPeriodBackOff(restartPeriodBackOff);
    retryPolicy.MutableMinRestartPeriod()->CopyFrom(
        NProtoInterop::CastToProto(minRestartPeriod)
    );
    retryPolicy.MutableMaxRestartPeriod()->CopyFrom(
        NProtoInterop::CastToProto(maxRestartPeriod)
    );

    return retryPolicy;
}

NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendOncePolicy GetSendOncePolicy(
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy& retryPolicy
) {
    NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendOncePolicy sendOncePolicy;

    sendOncePolicy.MutableRetryPolicy()->CopyFrom(retryPolicy);

    return sendOncePolicy;
}

NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy GetSendPeriodicallyPolicy(
    TDuration period
) {
    NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy sendPeriodicallyPolicy;

    sendPeriodicallyPolicy.MutablePeriod()->CopyFrom(
        NProtoInterop::CastToProto(period)
    );

    return sendPeriodicallyPolicy;
}

} // namespace

TEST(TMatrixWorkerServiceUtilsTest, TestGetNextTimeByRetryPolicySimple) {
    TInstant now = TInstant::Seconds(14);

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Zero(),
                0,
                TDuration::Seconds(5),
                TDuration::Seconds(15)
            ),
            0
        ),
        // 14 + 5
        TInstant::Seconds(19)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Seconds(3),
                0,
                TDuration::Seconds(5),
                TDuration::Seconds(15)
            ),
            0
        ),
        // 14 + 5 + 3 * 0**0
        TInstant::Seconds(22)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Seconds(3),
                12345,
                TDuration::Seconds(5),
                TDuration::Seconds(15)
            ),
            0
        ),
        // 14 + 5 + 3 * 12345**0
        TInstant::Seconds(22)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Seconds(100),
                0,
                TDuration::Seconds(5),
                TDuration::Seconds(15)
            ),
            0
        ),
        // 14 + min(15, 5 + 100)
        TInstant::Seconds(29)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Seconds(15),
                4,
                TDuration::Seconds(2),
                TDuration::Seconds(2000)
            ),
            1
        ),
        // 14 + 2 + 15 * 4**1
        TInstant::Seconds(76)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            now,
            GetRetryPolicy(
                TDuration::Seconds(9),
                7,
                TDuration::Seconds(12345),
                TDuration::Max()
            ),
            9
        ),
        // 14 + 12345 + 9 * 7**9
        TInstant::Seconds(363194822)
    );
}

TEST(TMatrixWorkerServiceUtilsTest, TestGetNextTimeByRetryPolicyCornerCases) {
    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            TInstant::Seconds(1),
            GetRetryPolicy(
                TDuration::Seconds(9),
                7,
                TDuration::Seconds(12345),
                TDuration::Max()
            ),
            Max<ui64>()
        ),
        TInstant::Max()
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            TInstant::Seconds(1),
            GetRetryPolicy(
                TDuration::Hours(293142),
                7,
                TDuration::Seconds(12345),
                TDuration::Max()
            ),
            20
        ),
        TInstant::Max()
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            TInstant::Max(),
            GetRetryPolicy(
                TDuration::Zero(),
                0,
                TDuration::Seconds(1),
                TDuration::Seconds(5)
            ),
            0
        ),
        TInstant::Max()
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            TInstant::Max() - TDuration::Seconds(2),
            GetRetryPolicy(
                TDuration::Zero(),
                0,
                TDuration::Seconds(1),
                TDuration::Seconds(5)
            ),
            0
        ),
        TInstant::Max() - TDuration::Seconds(1)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeByRetryPolicy(
            TInstant::Seconds(1),
            GetRetryPolicy(
                TDuration::Hours(12),
                9283742233453ull,
                TDuration::Seconds(1),
                TDuration::Hours(431231)
            ),
            39884754354ull
        ),
        TInstant::Seconds(1) + TDuration::Hours(431231)
    );
}

TEST(TMatrixWorkerServiceUtilsTest, TestGetNextRetryTimeBySendOncePolicy) {
    TInstant now = TInstant::Seconds(14);

    EXPECT_EQ(
         NMatrix::NWorker::GetNextRetryTimeBySendOncePolicy(
            now,
            GetSendOncePolicy(
                GetRetryPolicy(
                    TDuration::Zero(),
                    0,
                    TDuration::Seconds(5),
                    TDuration::Seconds(15)
                )
            ),
            0
        ),
        TInstant::Seconds(19)
    );

    EXPECT_EQ(
         NMatrix::NWorker::GetNextRetryTimeBySendOncePolicy(
            now,
            GetSendOncePolicy(
                GetRetryPolicy(
                    TDuration::Seconds(9),
                    7,
                    TDuration::Seconds(12345),
                    TDuration::Max()
                )
            ),
            9
        ),
        TInstant::Seconds(363194822)
    );
}

TEST(TMatrixWorkerServiceUtilsTest, TestGetNextRetryTimeBySendPeriodicallyPolicy) {
    TInstant now = TInstant::Seconds(14);

    // TODO(chegoryu) Add retry policy to SendPeriodicallyPolicy
    EXPECT_EQ(
         NMatrix::NWorker::GetNextRetryTimeBySendPeriodicallyPolicy(
            now,
            GetSendPeriodicallyPolicy(
                TDuration::Seconds(17)
            ),
            0
        ),
        TInstant::Seconds(31)
    );
}

TEST(TMatrixWorkerServiceUtilsTest, TestGetNextTimeBySendPeriodicallyPolicy) {
    TInstant now = TInstant::Seconds(14);

    EXPECT_EQ(
         NMatrix::NWorker::GetNextTimeBySendPeriodicallyPolicy(
            now,
            GetSendPeriodicallyPolicy(
                TDuration::Seconds(17)
            )
        ),
        TInstant::Seconds(31)
    );
}
