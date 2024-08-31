#include <alice/cachalot/library/modules/activation/yql_requests.h>

#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/string/builder.h>
#include <util/string/printf.h>


namespace NCachalot {

bool TActivationStorageYqlCodesOptions::operator==(const TActivationStorageYqlCodesOptions& other) const {
    return DatabaseName == other.DatabaseName && Flags.CombineFlags() == other.Flags.CombineFlags();
}


void TActivationStorageYqlCodesStorage::Initialize(const TString& databaseName) {
    GetCodes({
        .Flags = {.IgnoreRms = false},
        .DatabaseName = databaseName
    });
    GetCodes({
        .Flags = {.IgnoreRms = true},
        .DatabaseName = databaseName
    });
}

TActivationStorageYqlCodesPtr TActivationStorageYqlCodesStorage::GetCodes(
    const TActivationStorageYqlCodesOptions& options
) {
    {
        TReadGuard readGuard(Lock);
        if (const auto* ptr = Config2Codes.FindPtr(options)) {
            return *ptr;
        }
    }

    {
        // We should not go here at runtime.
        // All codes should be generated in TActivationStorageYqlCodesStorage::Initialize at service setup.
        TActivationStorageYqlCodesPtr result = MakeCodes(options);
        TWriteGuard writeGuard(Lock);
        Config2Codes[options] = result;
        return result;
    }
}

TActivationStorageYqlCodesStorage& TActivationStorageYqlCodesStorage::GetInstance() {
    return *Singleton<TActivationStorageYqlCodesStorage>();
}

TActivationStorageYqlCodesPtr MakeCodes(const TActivationStorageYqlCodesOptions options) {
    // WARNING!
    // static variables must not depend on $options parameter!
    // static + static -> static
    // static + $options -> non-static
    // Please, keep all statics in the beginning of this function.

    // NOTE:
    // First and last parentheses inside of R"()" are parts of C++ syntax.
    // R"(hello)"[0] == 'h'
    // R"((hello))"[0] == '('

    auto result = MakeIntrusive<TActivationStorageYqlCodes>();

    static const TString userRecordsCondition = "(UserIdHash == $user_id_hash AND UserId == $user_id)";

    static const TString actualUserRecordsCondition = Sprintf(
        R"((
            %s
                AND
            ActivationAttemptTime >= $freshness_threshold
        ))",
        userRecordsCondition.c_str()
    );

    static const TString previousActivationOfThisDevice = (
        R"((DeviceId == $device_id AND ActivationAttemptTime != $activation_attempt_time))"
    );

    static const TString сleanupLeadersYqlStatement = Sprintf(
        R"(
            DELETE FROM activation_leaders ON
            SELECT * FROM (
                SELECT * FROM activation_leaders
                WHERE (
                    %s
                        AND
                    (
                        ActivationAttemptTime < $freshness_threshold
                            OR
                        %s
                    )
                )
            );
        )",
        userRecordsCondition.c_str(),
        previousActivationOfThisDevice.c_str()
    );

    static const TString basicComparator = R"(
        ActivationAttemptTime < $activation_attempt_time OR
        (
            ActivationAttemptTime == $activation_attempt_time AND
            DeviceId < $device_id
        )
    )";

    static const TString validSpotterSearch = Sprintf(
        R"(
            SELECT
                DeviceId
            FROM activation_announcements
            WHERE
                %s AND
                SpotterValidated
            LIMIT 1
            ;
        )",
        actualUserRecordsCondition.c_str()
    );

    static const TString validSpotterAssertionAndSearch = Sprintf(
        R"(
            DISCARD SELECT Ensure(
                0,
                COUNT(*) > 0,
                "There are no validated records"
            ) FROM (
                SELECT
                    DeviceId
                FROM activation_announcements
                WHERE
                    (%s) AND
                    SpotterValidated
                LIMIT 1
            );

            %s
        )",
        actualUserRecordsCondition.c_str(),
        validSpotterSearch.c_str()
    );

    const TString tablePathPrefix = options.DatabaseName + "/v2";

    result->Annoncement = Sprintf(
        R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");

            DECLARE $activation_attempt_time AS Timestamp;
            DECLARE $avg_rms AS Float;
            DECLARE $device_id AS String;
            DECLARE $freshness_threshold AS Timestamp;
            DECLARE $spotter_validated AS Bool;
            DECLARE $ttl AS Timestamp;
            DECLARE $user_id AS String;
            DECLARE $user_id_hash AS Uint64;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS,
                SpotterValidated
            FROM activation_announcements
            WHERE
                (
                    %s
                )
                AND
                (
                    (
                        (
                            Math::FuzzyEquals(AvgRMS, 0) OR
                            Math::FuzzyEquals($avg_rms, 0) OR
                            Math::FuzzyEquals(AvgRMS, $avg_rms)
                        ) AND
                        (
                            %s
                        )

                    ) OR
                    (
                        (
                            NOT (
                                Math::FuzzyEquals(AvgRMS, 0) OR
                                Math::FuzzyEquals($avg_rms, 0)
                            )
                        ) AND
                        AvgRMS > $avg_rms
                    )
                )
            LIMIT 1;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS,
                SpotterValidated
            FROM activation_announcements
            WHERE %s
            ORDER BY AvgRMS DESC, ActivationAttemptTime ASC, DeviceId ASC
            LIMIT 1;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS,
                SpotterValidated
            FROM activation_announcements
            WHERE
                %s AND
                Math::FuzzyEquals(AvgRMS, 0)
            ORDER BY ActivationAttemptTime ASC, DeviceId ASC
            LIMIT 1;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS
            FROM activation_leaders
            WHERE %s AND NOT (%s)
            LIMIT 1;

            %s

            UPSERT INTO activation_announcements (
                UserIdHash,
                UserId,
                DeviceId,
                ActivationAttemptTime,
                AvgRMS,
                SpotterValidated,
                Deadline
            ) VALUES
                ($user_id_hash, $user_id, $device_id, $activation_attempt_time, $avg_rms, $spotter_validated, $ttl)
            ;
        )",
        tablePathPrefix.c_str(),
        actualUserRecordsCondition.c_str(),
        basicComparator.c_str(),
        actualUserRecordsCondition.c_str(),
        actualUserRecordsCondition.c_str(),
        actualUserRecordsCondition.c_str(),
        previousActivationOfThisDevice.c_str(),
        validSpotterSearch.c_str()
    );

    const TString comparator = options.Flags.IgnoreRms ? basicComparator : Sprintf(
        R"(
            AvgRMS > $avg_rms OR
            (
                Math::FuzzyEquals(AvgRMS, $avg_rms) AND
                (
                    %s
                )
            )
        )",
        basicComparator.c_str()
    );

    result->TryAcquireLeadership = Sprintf(
        // First of all we check whether better record appeared in announcement table.
        // If not, we drop old records and make unique record.

        // INSERT raises error when record is not uniqie by key.
        // We use this property as an identifier of success.

        R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");

            DECLARE $activation_attempt_time AS Timestamp;
            DECLARE $avg_rms AS Float;
            DECLARE $device_id AS String;
            DECLARE $freshness_threshold AS Timestamp;
            DECLARE $ttl AS Timestamp;
            DECLARE $user_id AS String;
            DECLARE $user_id_hash AS Uint64;

            %s

            DISCARD SELECT Ensure(
                0,
                COUNT(*) == 0,
                "There is better record"
            ) FROM (
                SELECT
                    DeviceId,
                    ActivationAttemptTime,
                    AvgRMS
                FROM activation_announcements
                WHERE
                    (
                        %s
                    )
                    AND
                    (
                        %s
                    )
                    AND
                    (
                        NOT (%s)
                    )
                LIMIT 1
            );

            INSERT INTO activation_leaders (
                UserIdHash,
                UserId,
                DeviceId,
                ActivationAttemptTime,
                AvgRMS,
                Deadline
            ) VALUES
                ($user_id_hash, $user_id, $device_id, $activation_attempt_time, $avg_rms, $ttl)
            ;
        )",
        tablePathPrefix.c_str(),
        validSpotterAssertionAndSearch.c_str(),
        actualUserRecordsCondition.c_str(),
        comparator.c_str(),
        previousActivationOfThisDevice.c_str()
    );

    result->GetLeader = Sprintf(
        // This is the best thing we can do if we want to predict the leader.
        // If the leader was not defined in the leaders table,
        // we search for the best spotter in the announcements table.
        R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");

            DECLARE $freshness_threshold AS Timestamp;
            DECLARE $user_id AS String;
            DECLARE $user_id_hash AS Uint64;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS
            FROM activation_leaders
            WHERE %s
            LIMIT 1
            ;

            SELECT
                DeviceId,
                ActivationAttemptTime,
                AvgRMS
            FROM activation_announcements
            WHERE %s
            ORDER BY %s
            LIMIT 1
            ;

            %s
        )",
        tablePathPrefix.c_str(),
        actualUserRecordsCondition.c_str(),
        actualUserRecordsCondition.c_str(),
        (
            options.Flags.IgnoreRms ?
                         "ActivationAttemptTime ASC, DeviceId ASC" :
            "AvgRMS DESC, ActivationAttemptTime ASC, DeviceId ASC"
        ),
        validSpotterSearch.c_str()
    );

    result->CleanupLeaders = Sprintf(
        R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");

            DECLARE $activation_attempt_time AS Timestamp;
            DECLARE $device_id AS String;
            DECLARE $freshness_threshold AS Timestamp;
            DECLARE $user_id AS String;
            DECLARE $user_id_hash AS Uint64;

            %s
        )",
        tablePathPrefix.c_str(),
        сleanupLeadersYqlStatement.c_str()
    );

    return result;
}

}   // namespace NCachalot
