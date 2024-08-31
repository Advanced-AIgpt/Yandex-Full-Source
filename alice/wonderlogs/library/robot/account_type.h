#pragma once

#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderlogs {

namespace NTestSuiteAccountType {

struct TTestCaseAccountType;
struct TTestCaseAccountTypeConstructor;

} // namespace NTestSuiteAccountType

class TAccountTypeInfo {
public:
    TAccountTypeInfo(TString uuid, TString messageId, TString megamindRequestId, TMaybe<TString> processId,
                     TMaybe<bool> locationYandexNet, TMaybe<bool> locationYandexStaff, TMaybe<bool> yandexNet,
                     TMaybe<bool> yandexStaff)
        : Uuid(std::move(uuid))
        , MessageId(std::move(messageId))
        , MegamindRequestId(std::move(megamindRequestId))
        , ProcessId(std::move(processId))
        , LocationYandexNet(locationYandexNet)
        , LocationYandexStaff(locationYandexStaff)
        , YandexNet(yandexNet)
        , YandexStaff(yandexStaff) {
    }

    explicit TAccountTypeInfo(const TWonderlog& wonderlog);

    [[nodiscard]] TWonderlog::EAccountType Type() const;

private:
    friend struct NAlice::NWonderlogs::NTestSuiteAccountType::TTestCaseAccountType;
    friend struct NAlice::NWonderlogs::NTestSuiteAccountType::TTestCaseAccountTypeConstructor;

    static bool GoodId(TStringBuf id);
    [[nodiscard]] bool Robot() const;
    [[nodiscard]] bool Staff() const;

    TString Uuid;
    TString MessageId;
    TString MegamindRequestId;
    TMaybe<TString> ProcessId;
    TMaybe<bool> LocationYandexNet;
    TMaybe<bool> LocationYandexStaff;
    TMaybe<bool> YandexNet;
    TMaybe<bool> YandexStaff;
};

} // namespace NAlice::NWonderlogs
