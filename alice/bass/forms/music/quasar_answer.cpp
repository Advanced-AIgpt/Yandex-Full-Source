#include "answers.h"

#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

namespace {

const TStringBuf FILTERS = "filters";

} // namespace

bool TQuasarMusicAnswer::Init(const NSc::TValue& result) {
    const NSc::TValue& object = result.Has("object") ? result["object"] : result.TrySelect("result/object");
    if (object.Has("type") && object.Has("value")) {
        Answer = object.Get("value");
        AnswerType = object.Get("type").GetString();

        if (AnswerType == "playlist" && Answer.Get("title").GetString() == "") {//TODO: add correct display on VINS side and remove this kostyl
            Answer = NSc::TValue();
            AnswerType = FILTERS;
        }
    } else {
        Answer = result.Has("result") ? result["result"] : NSc::Null();
        AnswerType = FILTERS;
    }
    return true;
}

bool TQuasarMusicAnswer::ConvertAnswerToOutputFormat(NSc::TValue* value) {
    if (AnswerType.empty()) {
        return false;
    }

    bool res = (AnswerType == FILTERS) ? MakeFiltersAnswer(value) : MakeOutputFromAnswer(value);
    (*value)["subtype"] = Answer.Get("type");
    (*value)["type"] = AnswerType;

    return res;
}

bool TQuasarMusicAnswer::MakeFiltersAnswer(NSc::TValue* value) {
    (*value)[FILTERS] = Answer.Clone();
    return true;
}

} // namespace NBASS::NMusic
