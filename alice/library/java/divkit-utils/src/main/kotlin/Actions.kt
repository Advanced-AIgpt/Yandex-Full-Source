package ru.yandex.alice.divkit

import com.yandex.div.dsl.LiteralProperty
import com.yandex.div.dsl.Property
import com.yandex.div.dsl.context.CardContext
import com.yandex.div.dsl.context.TemplateContext
import com.yandex.div.dsl.model.DivAction
import com.yandex.div.dsl.model.DivHover
import com.yandex.div.dsl.model.DownloadCallbacks
import com.yandex.div.dsl.model.divAction
import com.yandex.div.dsl.optionalValue
import com.yandex.div.dsl.value
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrameRequestData
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import java.net.URI

fun CardContext.tsfAction(
    logId: String,
    semanticFrame: TSemanticFrameRequestData,
    downloadCallbacks: DownloadCallbacks? = null,
    hover: DivHover? = null,
    logUrl: URI? = null,
    menuItems: List<DivAction.MenuItem>? = null,
    payload: Map<String, Any>? = null,
    referer: URI? = null,
    target: DivAction.Target? = null,
    dialogId: String? = null
): DivAction = directiveAction(
    logId = logId,
    directives = listOf(semanticFrame.toServerAction()),
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    dialogId = dialogId,
)

fun CardContext.directiveAction(
    logId: String,
    directive: TDirective,
    downloadCallbacks: DownloadCallbacks? = null,
    hover: DivHover? = null,
    logUrl: URI? = null,
    menuItems: List<DivAction.MenuItem>? = null,
    payload: Map<String, Any>? = null,
    referer: URI? = null,
    target: DivAction.Target? = null,
    dialogId: String? = null
): DivAction = directiveAction(
    logId = logId,
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    directives = listOfNotNull(directive.toClientAction()),
    dialogId = dialogId,
)

fun CardContext.directiveAction(
    logId: String,
    directive: Directive,
    downloadCallbacks: DownloadCallbacks? = null,
    hover: DivHover? = null,
    logUrl: URI? = null,
    menuItems: List<DivAction.MenuItem>? = null,
    payload: Map<String, Any>? = null,
    referer: URI? = null,
    target: DivAction.Target? = null,
    dialogId: String? = null
): DivAction = directiveAction(
    logId = logId,
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    directives = listOf(directive),
    dialogId = dialogId,
)

fun CardContext.directiveAction(
    logId: String,
    directives: List<Directive>,
    downloadCallbacks: DownloadCallbacks? = null,
    hover: DivHover? = null,
    logUrl: URI? = null,
    menuItems: List<DivAction.MenuItem>? = null,
    payload: Map<String, Any>? = null,
    referer: URI? = null,
    target: DivAction.Target? = null,
    dialogId: String? = null
): DivAction = divAction(
    logId = logId,
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    url = directives.takeIf { it.isNotEmpty() }?.let { action ->
        createDialogActionUri(directives = action, dialogId = dialogId)
    },
)

fun <T> TemplateContext<T>.tsfAction(
    semanticFrame: TSemanticFrameRequestData,
    logId: Property<String>? = null,
    downloadCallbacks: Property<DownloadCallbacks>? = null,
    hover: Property<DivHover>? = null,
    logUrl: Property<URI>? = null,
    menuItems: Property<List<DivAction.MenuItem>>? = null,
    payload: Property<Map<String, Any>>? = null,
    referer: Property<URI>? = null,
    target: Property<DivAction.Target>? = null,
    dialogId: String? = null
): LiteralProperty<DivAction> = directiveAction(
    logId = logId,
    directives = listOf(semanticFrame.toServerAction()),
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    dialogId = dialogId,
)

fun <T> TemplateContext<T>.directiveAction(
    directive: TDirective,
    logId: Property<String>? = null,
    downloadCallbacks: Property<DownloadCallbacks>? = null,
    hover: Property<DivHover>? = null,
    logUrl: Property<URI>? = null,
    menuItems: Property<List<DivAction.MenuItem>>? = null,
    payload: Property<Map<String, Any>>? = null,
    referer: Property<URI>? = null,
    target: Property<DivAction.Target>? = null,
    dialogId: String? = null
): LiteralProperty<DivAction> = directiveAction(
    logId = logId,
    directives = listOfNotNull(directive.toClientAction()),
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    dialogId = dialogId,
)

fun <T> TemplateContext<T>.directiveAction(
    directives: List<Directive>,
    logId: Property<String>? = null,
    downloadCallbacks: Property<DownloadCallbacks>? = null,
    hover: Property<DivHover>? = null,
    logUrl: Property<URI>? = null,
    menuItems: Property<List<DivAction.MenuItem>>? = null,
    payload: Property<Map<String, Any>>? = null,
    referer: Property<URI>? = null,
    target: Property<DivAction.Target>? = null,
    dialogId: String? = null,
): LiteralProperty<DivAction> = divAction(
    logId = logId,
    downloadCallbacks = downloadCallbacks,
    hover = hover,
    logUrl = logUrl,
    menuItems = menuItems,
    payload = payload,
    referer = referer,
    target = target,
    url = optionalValue(
        directives.takeIf { it.isNotEmpty() }?.let { action ->
            createDialogActionUri(directives = action, dialogId = dialogId)
        }
    ),
)

fun <T> TemplateContext<T>.createSetStateUrl(temporary: Boolean = false, vararg stateId: String): Property<URI> =
    value(createSetStateUrl(stateId, temporary))

fun CardContext.createSetStateUrl(temporary: Boolean = false, vararg stateId: String): URI =
    createSetStateUrl(stateId, temporary)

private fun createSetStateUrl(stateId: Array<out String>, temporary: Boolean): URI =
    URI.create("div-action://set_state?state_id=${stateId.joinToString(separator = "/")}&temporary=${temporary}")

