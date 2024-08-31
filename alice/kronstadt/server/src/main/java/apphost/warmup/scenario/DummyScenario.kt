package ru.yandex.alice.kronstadt.server.apphost.warmup.scenario

import com.google.protobuf.Empty
import com.google.protobuf.Message
import com.yandex.div.dsl.context.CardWithTemplates
import com.yandex.div.dsl.context.TemplateContext
import com.yandex.div.dsl.context.card
import com.yandex.div.dsl.context.define
import com.yandex.div.dsl.context.override
import com.yandex.div.dsl.context.resolve
import com.yandex.div.dsl.context.templates
import com.yandex.div.dsl.enum
import com.yandex.div.dsl.int
import com.yandex.div.dsl.model.Div
import com.yandex.div.dsl.model.DivAction
import com.yandex.div.dsl.model.DivAlignmentHorizontal
import com.yandex.div.dsl.model.DivAlignmentVertical
import com.yandex.div.dsl.model.DivContainer
import com.yandex.div.dsl.model.DivFontWeight
import com.yandex.div.dsl.model.DivTabs
import com.yandex.div.dsl.model.divAction
import com.yandex.div.dsl.model.divBorder
import com.yandex.div.dsl.model.divContainer
import com.yandex.div.dsl.model.divData
import com.yandex.div.dsl.model.divEdgeInsets
import com.yandex.div.dsl.model.divFixedSize
import com.yandex.div.dsl.model.divGallery
import com.yandex.div.dsl.model.divImage
import com.yandex.div.dsl.model.divImageBackground
import com.yandex.div.dsl.model.divMatchParentSize
import com.yandex.div.dsl.model.divSolidBackground
import com.yandex.div.dsl.model.divTabs
import com.yandex.div.dsl.model.divText
import com.yandex.div.dsl.model.divWrapContentSize
import com.yandex.div.dsl.model.item
import com.yandex.div.dsl.model.menuItem
import com.yandex.div.dsl.model.state
import com.yandex.div.dsl.model.tabTitleStyle
import com.yandex.div.dsl.model.template
import com.yandex.div.dsl.reference
import com.yandex.div.dsl.type.BoolInt
import com.yandex.div.dsl.type.color
import com.yandex.div.dsl.value
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionSpace
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.Features
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.PlayerFeatures
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.StackEngine
import ru.yandex.alice.kronstadt.core.StackEngineAction
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.kronstadt.core.directive.CloseDialogDirective
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective
import ru.yandex.alice.kronstadt.core.directive.server.DeletePusheDirective
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.directive.server.UserConfig
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.Theme
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.ApplyingScene
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.core.semanticframes.ButtonAnalytics
import ru.yandex.alice.kronstadt.core.semanticframes.MusicPlay
import ru.yandex.alice.kronstadt.core.semanticframes.ParsedUtterance
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.protos.data.ChildAge.TChildAge
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData
import ru.yandex.alice.protos.data.scenario.afisha.Afisha
import java.net.URI
import kotlin.reflect.KClass

val DUMMY_SCENARIO_META = ScenarioMeta("dummy_scenario", useDivRenderer = true)

@Component
internal class DummyScenario : AbstractNoStateScenario(DUMMY_SCENARIO_META) {

    private val logger = LogManager.getLogger()
    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        // to string method touches all the fields
        logger.trace(request.deviceState.toString())
        logger.trace(request.userRegion.toString())
        logger.trace(request.blackboxInfo.toString())
        logger.trace(request.iotUserInfo.toString())
        logger.trace(request.contactsList.toString())
        logger.trace(request.videoCallCapability.toString())
        logger.trace(request.contactsList.toString())
        logger.trace(request.toString())
        scene<DummyScene>()
    }

    override fun applyArgumentsToProto(applyArguments: ApplyArguments): Message = Empty.getDefaultInstance()
    override fun applyArgumentsFromProto(arg: com.google.protobuf.Any): ApplyArguments = EmptyData

    @Component
    internal class DummyScene : AbstractNoargScene<Any>("dummy_scene"), ApplyingScene<Any, Any, Empty> {
        override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
            return ApplyNeededResponse(
                arguments = EmptyData,
                features = Features(playerFeatures = PlayerFeatures(restorePlayer = true, secondsSincePause = 1))
            )
        }

        override val applyArgsClass: KClass<Empty> = Empty::class

        private fun createDirectives(): List<MegaMindDirective> = listOf(
            ShowViewDirective(div2Card = renderDistrictCard(), actionSpaceId = "foo", doNotShowCloseButton = true),
            OpenUriDirective("ya.ru"),
            TypeTextDirective("text"),
            CloseDialogDirective("123123"),
            //StartVideoCallLoginDirective(),
        )

        private fun createServerDirectives(): List<ServerDirective> = listOf(
            MementoChangeUserObjectsDirective(
                userConfigs = mapOf(
                    EConfigKey.CK_CHILD_AGE to UserConfig(
                        TChildAge.newBuilder()
                            .setAge(5).build()
                    )
                ),
            ),
            SendPushMessageDirective(
                title = "title",
                body = "body",
                link = URI.create("ya.ru"),
                pushId = "push_id",
                pushTag = "push_tag",
                throttlePolicy = "foo",
                cardButtonText = "card_button_text"
            ),
            DeletePusheDirective(
                pushTag = "tag"
            ),
        )

        override fun processApply(
            request: MegaMindRequest<Any>,
            args: Any,
            applyArg: Empty
        ): ScenarioResponseBody<Any> {
            return ScenarioResponseBody(
                layout = Layout(
                    outputSpeech = "текст",
                    shouldListen = true,
                    suggests = listOf(
                        Button(
                            text = "text",
                            url = null,
                            payload = "{}",
                            semanticFrame = SemanticFrame.create(
                                "frame",
                                SemanticFrameSlot.Companion.create("slotType", "value", "type")
                            ),
                            hide = false,
                            directives = createDirectives(),
                            nluFrameName = "name",
                            theme = Theme("ya.ru"),
                            typedSemanticFrame = MusicPlay,
                            parsedUtterance = ParsedUtterance(
                                "utterance",
                                frame = MusicPlay,
                                buttonAnalytics = ButtonAnalytics(
                                    purpose = "purpose",
                                    originInfo = "origin"
                                )
                            ),
                            actionId = null
                        )
                    )
                ),
                analyticsInfo = AnalyticsInfo(
                    "dummy",
                    actions = listOf(AnalyticsInfoAction(id = "id", name = "name", humanReadable = "hh")),
                    objects = listOf(AnalyticsInfoObject(id = "id", name = "name", humanReadable = "hh")),
                ),
                isExpectsRequest = false,
                state = null,
                serverDirectives = createServerDirectives(),
                stackEngine = StackEngine(StackEngineAction.NewSession),
                actionSpaces = mapOf("foo" to ActionSpace(mapOf(), nluHints = listOf())),
                renderData = listOf(
                    DivRenderData(
                        "card_id",
                        TScenarioData.newBuilder()
                            .setAfishaTeaserData(Afisha.TAfishaTeaserData.getDefaultInstance())
                            .build()
                    )
                )

            )
        }

        private fun renderDistrictCard(): CardWithTemplates {

            val districtButtonActionLinkRef = reference<URI>("district_button_action_link")
            val titleItemsRef = reference<List<Div>>("title_items")
            val tabItemsLinkRef = reference<List<DivTabs.Item>>("tab_items_link")
            val districtGalleryItemActionLinkRef = reference<URI>("district_gallery_item_action_link")
            val addressRef = reference<String>("address")
            val messageRef = reference<String>("message")
            val avatarUrlRef = reference<URI>("avatar_url")
            val usernameRef = reference<String>("username")
            val imageCountRef = reference<String>("image_count")
            val commentCountRef = reference<String>("comment_count")
            val galleryTailActionLinkRef = reference<DivAction>("gallery_tail_action_link")
            val tailTextLinkRef = reference<String>("tail_text_link")

            val templates: TemplateContext<Div> = templates<Div> {
                define(
                    "title_text",
                    divText(
                        paddings = divEdgeInsets(
                            left = int(16),
                            top = int(12),
                            right = int(16),
                        ),
                        fontSize = int(18),
                        lineHeight = int(24),
                        textColor = value("#CC000000".color),
                        fontWeight = enum(DivFontWeight.MEDIUM)
                    )
                )
                define(
                    "title_menu",
                    divImage(
                        width = divFixedSize(value = int(44)),
                        height = divFixedSize(value = int(44)),
                        imageUrl = value(URI.create("https://i.imgur.com/qNJQKU8.png")),
                    )
                )
                define(
                    "district_button",
                    divText(
                        text = value("Написать новость"),
                        fontSize = int(14),
                        border = divBorder(cornerRadius = int(4)),
                        margins = divEdgeInsets(
                            left = int(16),
                            right = int(16),
                            top = int(12),
                        ),
                        paddings = divEdgeInsets(
                            bottom = int(8),
                            top = int(8),
                        ),
                        action = divAction(
                            url = districtButtonActionLinkRef,
                            logId = value("new_post"),
                        ),
                        textAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
                        background = listOf(divSolidBackground(color = value("#ffdc60".color))),
                    )
                )
                define(
                    "district_card",
                    divContainer(
                        orientation = enum(DivContainer.Orientation.VERTICAL),
                        paddings = divEdgeInsets(bottom = int(16)),
                        height = divWrapContentSize(),
                        items = listOf(
                            divContainer(
                                orientation = enum(DivContainer.Orientation.HORIZONTAL),
                                items = titleItemsRef,
                            ),
                            divTabs(
                                switchTabsByContentSwipeEnabled = enum(BoolInt.FALSE),
                                titlePaddings = divEdgeInsets(
                                    left = int(12),
                                    right = int(12),
                                    bottom = int(8),
                                ),
                                tabTitleStyle = tabTitleStyle(
                                    fontWeight = enum(DivFontWeight.MEDIUM)
                                ),
                                items = tabItemsLinkRef
                            ),
                            template("district_button"),
                        ),
                        background = listOf(
                            divImageBackground(
                                imageUrl = value(URI.create("https://api.yastatic.net/morda-logo/i/yandex-app/district/district_day.3.png")),
                                preloadRequired = enum(BoolInt.TRUE),
                            )
                        )
                    )
                )
                define(
                    "district_gallery_item",
                    divContainer(
                        orientation = enum(DivContainer.Orientation.VERTICAL),
                        width = divFixedSize(value = int(272)),
                        height = divFixedSize(value = int(204)),
                        border = divBorder(
                            cornerRadius = int(6)
                        ),
                        paddings = divEdgeInsets(
                            left = int(12),
                            right = int(12),
                            top = int(0),
                            bottom = int(0),
                        ),
                        background = listOf(
                            divSolidBackground(
                                color = value("#ffffff".color)
                            )
                        ),
                        action = divAction(
                            logId = value("district_item"),
                            url = districtGalleryItemActionLinkRef
                        ),
                        items = listOf(
                            divText(
                                text = addressRef,
                                fontSize = int(13),
                                textColor = value("#80000000".color),
                                lineHeight = int(16),
                                maxLines = int(1),
                                height = divWrapContentSize(),
                                paddings = divEdgeInsets(
                                    left = int(0),
                                    right = int(0),
                                    top = int(12),
                                    bottom = int(0),
                                ),
                            ),
                            divText(
                                text = messageRef,
                                alignmentVertical = enum(DivAlignmentVertical.TOP),
                                fontSize = int(14),
                                lineHeight = int(20),
                                maxLines = int(6),
                                height = divMatchParentSize(),
                                paddings = divEdgeInsets(
                                    top = int(5),
                                    bottom = int(5),
                                ),
                            ),
                            divContainer(
                                orientation = enum(DivContainer.Orientation.HORIZONTAL),
                                contentAlignmentVertical = enum(DivAlignmentVertical.CENTER),
                                paddings = divEdgeInsets(
                                    bottom = int(12),
                                ),
                                items = listOf(
                                    divImage(
                                        imageUrl = avatarUrlRef,
                                        width = divFixedSize(value = int(24)),
                                        height = divFixedSize(value = int(24)),
                                        border = divBorder(
                                            cornerRadius = int(12),
                                        ),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        preloadRequired = enum(BoolInt.TRUE),
                                    ),
                                    divText(
                                        text = usernameRef,
                                        fontSize = int(13),
                                        fontWeight = enum(DivFontWeight.MEDIUM),
                                        textColor = value("#80000000".color),
                                        lineHeight = int(16),
                                        maxLines = int(1),
                                        width = divFixedSize(value = int(126)),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        paddings = divEdgeInsets(
                                            left = int(8),
                                            right = int(8),
                                        ),
                                    ),
                                    divImage(
                                        imageUrl = value(URI.create("https://api.yastatic.net/morda-logo/i/yandex-app/district/images_day.1.png")),
                                        width = divFixedSize(value = int(24)),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        height = divFixedSize(value = int(24)),
                                        placeholderColor = value("#00ffffff".color),
                                        preloadRequired = enum(BoolInt.TRUE),
                                    ),
                                    divText(
                                        text = imageCountRef,
                                        fontSize = int(13),
                                        fontWeight = enum(DivFontWeight.MEDIUM),
                                        textColor = value("#80000000".color),
                                        lineHeight = int(16),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        maxLines = int(1),
                                        paddings = divEdgeInsets(
                                            right = int(10),
                                            left = int(4),
                                        ),
                                    ),
                                    divImage(
                                        imageUrl = value(URI.create("https://api.yastatic.net/morda-logo/i/yandex-app/district/images_day.1.png")),
                                        width = divFixedSize(value = int(24)),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        height = divFixedSize(value = int(24)),
                                        placeholderColor = value("#00ffffff".color),
                                        preloadRequired = enum(BoolInt.TRUE),
                                    ),
                                    divText(
                                        text = commentCountRef,
                                        fontSize = int(13),
                                        alignmentVertical = enum(DivAlignmentVertical.CENTER),
                                        fontWeight = enum(DivFontWeight.MEDIUM),
                                        textColor = value("#80000000".color),
                                        lineHeight = int(16),
                                        maxLines = int(1),
                                        paddings = divEdgeInsets(
                                            left = int(4)
                                        ),
                                    )

                                )

                            )
                        )
                    )
                )
                define(
                    "gallery_tail",
                    divContainer(
                        width = divFixedSize(value = int(104)),
                        height = divMatchParentSize(),
                        action = galleryTailActionLinkRef,
                        contentAlignmentVertical = enum(DivAlignmentVertical.CENTER),
                        contentAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
                        items = listOf(
                            divImage(
                                imageUrl = value(URI.create("https://i.imgur.com/CPmGi24.png")),
                                width = divFixedSize(value = int(40)),
                                height = divFixedSize(value = int(40)),
                                border = divBorder(
                                    cornerRadius = int(20),
                                ),
                                background = listOf(
                                    divSolidBackground(
                                        color = value("#ffffff".color)
                                    )
                                ),
                                placeholderColor = value("#00ffffff".color),
                                preloadRequired = enum(BoolInt.TRUE),
                            ),
                            divText(
                                text = tailTextLinkRef,
                                fontSize = int(14),
                                textColor = value("#6b7a80".color),
                                lineHeight = int(16),
                                textAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
                                height = divWrapContentSize(),
                                paddings = divEdgeInsets(
                                    left = int(0),
                                    right = int(0),
                                    top = int(10),
                                    bottom = int(0),
                                )
                            )
                        )
                    )
                )
            }
            val card = card {
                divData(
                    logId = "district_card",
                    states = listOf(
                        state(
                            stateId = 1,
                            div = template(
                                type = "district_card",
                                resolve(
                                    titleItemsRef,
                                    listOf(
                                        template(
                                            type = "title_text",
                                            override("text", "Новости района")
                                        ),
                                        template(
                                            type = "title_menu",
                                            override(
                                                "action",
                                                divAction(
                                                    logId = "menu",
                                                    menuItems = listOf(
                                                        menuItem(
                                                            text = "Настройки ленты",
                                                            action = divAction(
                                                                logId = "settings",
                                                                url = URI.create("http://ya.ru"),
                                                            )
                                                        ),
                                                        menuItem(
                                                            text = "Скрыть карточку",
                                                            action = divAction(
                                                                logId = "hide",
                                                                url = URI.create("http://ya.ru"),
                                                            )
                                                        )
                                                    )
                                                )
                                            )
                                        )
                                    )
                                ),
                                resolve(districtButtonActionLinkRef, URI.create("http://ya.ru")),
                                resolve(
                                    tabItemsLinkRef, listOf(
                                        item(
                                            title = "ПОПУЛЯРНОЕ",
                                            div = divGallery(
                                                width = divMatchParentSize(40.0),
                                                height = divFixedSize(value = 204),
                                                paddings = divEdgeInsets(
                                                    left = 16,
                                                    right = 16,
                                                ),
                                                items = listOf(
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо если"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо…"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо…"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо…"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо…"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "gallery_tail",
                                                        resolve(tailTextLinkRef, "Все новости района")
                                                    )
                                                )
                                            )
                                        ),
                                        item(
                                            title = "ВСЕ",
                                            div = divGallery(
                                                width = divMatchParentSize(40.0),
                                                height = divFixedSize(value = 204),
                                                paddings = divEdgeInsets(
                                                    left = 16,
                                                    right = 16,
                                                ),
                                                items = listOf(
                                                    template(
                                                        type = "district_gallery_item",
                                                        resolve(addressRef, "Ул. Льва Толстого, 16"),
                                                        resolve(
                                                            messageRef,
                                                            "Каток в парке Горького подготовился серьезно к началу сезона.\nБудет три катка: основная площадка, детский каток и для любителей хоккея. А в выходные каток будет открыт до полуночи. И еще хорошо…"
                                                        ),
                                                        resolve(
                                                            avatarUrlRef,
                                                            URI.create("https://avatars.mds.yandex.net/get-yapic/40841/520495100-1548277370/islands-200")
                                                        ),
                                                        resolve(usernameRef, "igorbuschina"),
                                                        resolve(imageCountRef, "3"),
                                                        resolve(commentCountRef, "1"),
                                                        resolve(
                                                            districtGalleryItemActionLinkRef,
                                                            URI.create("http://ya.ru")
                                                        )
                                                    ),
                                                    template(
                                                        type = "gallery_tail",
                                                        resolve(tailTextLinkRef, "Все новости района")
                                                    )
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            }

            return CardWithTemplates(card, templates)
        }
    }

    object EmptyData : ApplyArguments
}
