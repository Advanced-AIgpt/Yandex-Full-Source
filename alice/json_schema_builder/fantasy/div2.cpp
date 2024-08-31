#include "div2.h"

namespace NAlice::NJsonSchemaBuilder::NFantasy {

NAlice::NJsonSchemaBuilder::NRuntime::TValidCard Validate(
    const NAlice::NJsonSchemaBuilder::NRuntime::TTemplates& templates,
    NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& builder)
{
    return {
        templates.ValueWithoutValidation(),  // TODO(a-square): filter templates
        std::move(builder).ValueWithoutValidation(),  // TODO(a-square): validate the card
    };
}

}  // namespace NAlice::NJsonSchemaBuilder::NFantasy
