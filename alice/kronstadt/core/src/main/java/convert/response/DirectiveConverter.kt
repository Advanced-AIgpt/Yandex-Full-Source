package ru.yandex.alice.kronstadt.core.convert.response

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

interface DirectiveConverter : ToProtoConverter<MegaMindDirective, TDirective>
