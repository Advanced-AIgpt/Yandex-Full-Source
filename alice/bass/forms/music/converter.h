#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/scheme/scheme.h>

class QuasarRequestConverter {
public:
    explicit QuasarRequestConverter(const NSc::TValue& requestBody)
        : RequestBody(requestBody)
    {
    }

    NAlice::TTypedSemanticFrame&& GetTypedSemanticFrame() {
        return std::move(TypedSemanticFrame);
    }

    [[nodiscard]] bool Convert();

    void SetAlarmId(const TStringBuf alarmId);

private:
    [[nodiscard]] bool ConvertObject();
    [[nodiscard]] bool ApplyModifiers();

private:
    const NSc::TValue& RequestBody;
    NAlice::TTypedSemanticFrame TypedSemanticFrame;
};
