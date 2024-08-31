#include "messages.h"
#include "messages_en.h"
#include "messages_ru.h"
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>

namespace NGranet::NCompiler {

TMessageTable::TMessageTable(std::initializer_list<std::pair<EMessageId, TStringBuf>> table)
{
    for (const auto& [id, str] : table) {
        const EMessageId expectedId = static_cast<EMessageId>(Table.ysize());
        Y_ENSURE(id == expectedId, "Can not find message string for " << ToString(expectedId));
        Table.push_back(TString{str});
    }
    Y_ENSURE(Table.size() == GetEnumAllValues<EMessageId>().size());
}

const TMessageTable& GetMessageTable(ELanguage lang) {
    if (lang == LANG_RUS) {
        return MESSAGE_TABLE_RU;
    } else {
        return MESSAGE_TABLE_EN;
    }
}

} // namespace NGranet::NCompiler
