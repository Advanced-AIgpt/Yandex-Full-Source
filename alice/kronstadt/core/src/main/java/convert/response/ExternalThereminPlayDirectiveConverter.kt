package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.ExternalThereminPlayDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TThereminPlayDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TThereminPlayDirective.TExternalSet
import ru.yandex.alice.megamind.protos.scenarios.directive.TThereminPlayDirective.TSample

@Component
class ExternalThereminPlayDirectiveConverter : DirectiveConverterBase<ExternalThereminPlayDirective> {
    override fun convert(src: ExternalThereminPlayDirective, ctx: ToProtoContext): TDirective {
        val samples: List<TSample> = src.sampleUrls.map { url -> TSample.newBuilder().setUrl(url).build() }

        val externalSet = TExternalSet.newBuilder()
            .setNoOverlaySamples(src.noOverlaySamples)
            .setStopOnCeil(src.stopOnCeil)
            .setRepeatSoundInside(src.repeatSoundInside)
            .addAllSamples(samples)
            .build()
        val directive = TThereminPlayDirective.newBuilder()
            .setExternalSet(externalSet)
        return TDirective.newBuilder()
            .setThereminPlayDirective(directive)
            .build()
    }

    override val directiveType: Class<ExternalThereminPlayDirective>
        get() = ExternalThereminPlayDirective::class.java
}
