#pragma once

#include <alice/megamind/library/models/interfaces/model.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

enum class EButtonType {
    Action /* "action" */,
    ThemedAction /* "themed_action" */,
};

class IButtonModel : public IModel, public virtual TThrRefBase {
public:
    [[nodiscard]] virtual const TString& GetTitle() const = 0;
    [[nodiscard]] virtual EButtonType GetType() const = 0;
};

} // namespace NAlice::NMegamind
