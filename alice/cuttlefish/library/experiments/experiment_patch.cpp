#include "experiment_patch.h"
#include "patch_functions.h"
#include "utils.h"
#include <util/system/yassert.h>
#include <type_traits>
#include <exception>


namespace NVoice::NExperiments {

namespace {

// Wrap around TJsonValue that is implicitly convertible to any possible type
class TJsonArgument {
public:
    TJsonArgument(const NJson::TJsonValue& value)
        : Value(value)
    { }

    // Here must be all neccessary conversion operators
    operator const NJson::TJsonValue&() const           { return Value; }
    operator const NJson::TJsonValue::TArray&() const   { return Value.GetArraySafe(); }
    operator int() const                                { return Value.GetIntegerSafe(); }
    operator double() const                             { return Value.GetDoubleSafe(); }
    operator const TString&() const                     { return Value.GetStringSafe(); }

private:
    const NJson::TJsonValue& Value;
};


using TJsonIterator = NJson::TJsonValue::TArray::const_iterator;


template <class T, class ...TArgs>
THolder<T> MakePatchFunctionHolder(TJsonIterator it, TArgs&& ...args) {
    if constexpr (sizeof...(TArgs) == ConstructorArgumentsMinCount<T>) {
        return MakeHolder<T>(std::forward<TArgs>(args)...);
    } else {
        return MakePatchFunctionHolder<T>(it + 1, std::forward<TArgs>(args)..., TJsonArgument(*it));
    }
}

template <class T>
THolder<IPatchFunction> Create(const TExpContext&, TJsonIterator& it, const TJsonIterator& end)
{
    Y_ENSURE(
        std::size_t(end - it) >= ConstructorArgumentsMinCount<T>,
        "Too few arguments to construct experiment patch function"
    );

    THolder<T> res = MakePatchFunctionHolder<T>(it);
    it += ConstructorArgumentsMinCount<T>;
    return res;
}

template <>
THolder<IPatchFunction> Create<TImportMacro>(const TExpContext& expContext, TJsonIterator& it, const TJsonIterator& end)
{
    Y_ENSURE(
        std::size_t(end - it) >= 2,
        "Too few arguments to construct experiment patch function"
    );

    THolder<IPatchFunction> res = MakePatchFunctionHolder<TImportMacro>(it, expContext);
    it += 2;
    return res;
}

using TCreateFuncPtr = THolder<IPatchFunction>(*)(const TExpContext&, TJsonIterator&, const TJsonIterator&);

TCreateFuncPtr GetPatchFunctionCreator(const TStringBuf funcName) {
    static const THashMap<TStringBuf, TCreateFuncPtr> PATCH_FUNCTIONS({
        {"if_event_type", Create<TIfEventType>},
        {"if_has_staff_login", Create<TIfHasStaffLogin>},
        {"if_staff_login_eq", Create<TIfStaffLoginEq>},
        {"if_staff_login_in", Create<TIfStaffLoginIn>},
        {"if_has_payload", Create<TIfHasPayload>},
        {"if_payload_eq", Create<TIfPayloadEq>},
        {"if_payload_ne", Create<TIfPayloadNe>},
        {"if_payload_in", Create<TIfPayloadIn>},
        {"if_payload_like", Create<TIfPayloadLike>},
        {"if_has_session_data", Create<TIfHasSessionData>},
        {"if_session_data_eq", Create<TIfSessionDataEq>},
        {"if_session_data_ne", Create<TIfSessionDataNe>},
        {"if_session_data_in", Create<TIfSessionDataIn>},
        {"if_session_data_like", Create<TIfSessionDataLike>},
        {"set", Create<TSet>},
        {"set_if_none", Create<TSetIfNone>},
        {"append", Create<TAppend>},
        {"del", Create<TDel>},
        {"import_macro", Create<TImportMacro>},
        {"extend", Create<TExtend>}
    });

    const auto it = PATCH_FUNCTIONS.find(funcName);
    Y_ENSURE(it != PATCH_FUNCTIONS.end(), "Patch function '" << funcName << "' doesn't exist");
    return it->second;
}

} // anoynmous namespace
// ------------------------------------------------------------------------------------------------

TExpPatch::TExpPatch(const TExpContext& expContext, TPatchFunctions&& funcs)
    : ExpContext(expContext)
    , Funcs(std::move(funcs))
{ }

TExpPatch::TExpPatch(const TExpContext& expContext, const NJson::TJsonValue::TArray& array)
    : ExpContext(expContext)
    , Funcs()
{
    auto it = array.cbegin();
    const auto end = array.cend();
    while (it != end) {
        const TStringBuf funcName = it->GetStringSafe();
        THolder<IPatchFunction> func = GetPatchFunctionCreator(funcName)(ExpContext, ++it, end);
        Y_ENSURE(func);  // must not be nullptr
        Funcs.push_back(std::move(func));
    }
}

bool TExpPatch::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& context) const
{
    /**
     * NOTE: All patch functions are validated in their constructors but it still may be `bad_alloc`
     * or something alike thrown. If this happens applied patches won't be reverted and target
     * event will stay in semi-patched state.
     */

    for (const auto& funcPtr : Funcs) {
        if (!funcPtr->Apply(event, context)) {
            return false;
        }
    }
    return true;
}

} // namespace NVoice::NExperiments
