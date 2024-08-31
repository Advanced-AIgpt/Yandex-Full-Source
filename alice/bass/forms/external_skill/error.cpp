#include "error.h"

#include <alice/bass/forms/context/context.h>

#include <util/string/subst.h>

namespace NBASS {
namespace NExternalSkill {

const TErrorBlock::TResult TErrorBlock::Ok;

TErrorBlock::TErrorBlock(TError::EType type, TStringBuf errmsg)
    : Error(type, errmsg)
{
}

TErrorBlock::TErrorBlock(TStringBuf errmsg, TStringBuf type, TStringBuf path)
    : Error(TError::EType::SKILLSERROR, errmsg)
{
    AddProblem(type, path);
}

TErrorBlock::TErrorBlock(TError error)
    : Error(error)
{
}

void TErrorBlock::InsertIntoContex(TContext* ctx) const {
    Y_ASSERT(ctx);

    if (IsDeveloperMode) {
        TSlot* const slot = ctx->CreateSlot("response", "response", false);

        TString json{Data.ToJsonPretty()};
        SubstGlobal(json, "\n", "\\n");
        SubstGlobal(json, " ", "·");
        slot->Value["text"].SetString(json);
        slot->Value["voice"].SetString("произошла ошибка, смотрите техническую информацию");
    }
    else {
        ctx->AddErrorBlock(Error, Data);
    }
}

TErrorBlock& TErrorBlock::AddProblem(TStringBuf type, TStringBuf path, NSc::TValue data) {
    NSc::TValue& problem = Data["problems"].Push(data);
    problem["type"].SetString(type);
    problem["path"].SetString(path);
    return *this;
}

TErrorBlock& TErrorBlock::AddProblem(TStringBuf type, TStringBuf path) {
    return AddProblem(type, path, NSc::Null());
}

} // namespace NExternalSkill
} // namespace NBASS
