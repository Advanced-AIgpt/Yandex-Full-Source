#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

enum class EOpenExternalAppResult {
    UnableOpenRequestedApp,     // невозможно открыть запрошенное приложение
    OpenRequestedApp,           // Запрошенное приложение найдено среди установленных и будет открыто
    OpenRequestedAppStorePage,  // запрошенное приложение не найдено среди установленных, откроется стор со страницей установки приложения
    UnableOpenUnlistedApp       // запрошенного приложения нету ни на устройстве ни в сторе
};

namespace NSmartDevice::NExternalApp::NSlots {
    constexpr TStringBuf APPLICATION_NAME = "application";
}

} // namespace NAlice::NHollywood
