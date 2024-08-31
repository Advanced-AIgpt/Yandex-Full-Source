package ru.yandex.alice.divkttemplates.feature_boarding

import com.yandex.div.dsl.*
import com.yandex.div.dsl.context.*
import com.yandex.div.dsl.model.*
import com.yandex.div.dsl.type.BoolInt
import com.yandex.div.dsl.type.color
import java.net.URI

object NoTandemDevicePromoTemplate {
    private val newIconIdRef = reference<String>("icon_id")
    private val newIconAlphaRef = reference<Double>("new_visible")

    private val boardingTitleIdRef = reference<String>("title_id")
    private val boardingTitleTextRef = reference<String>("title_text")

    private val boardingDescriptionIdRef = reference<String>("description_id")
    private val boardingDescriptionTextRef = reference<String>("description_text")

    private val boardingButtonTextRef = reference<String>("button_text")
    private val boardingButtonActionUrlRef = reference<URI>("button_action")
    private val boardingButtonActionLogIdRef = reference<String>("button_log_id")
    private val boardingButtonActionPayloadRef = reference<Map<String, Any>>("action_payload")

    private val boardingCardBackgroundRef = reference<List<DivBackground>>("backgrounds")
    private val boardingCardIdRef = reference<String>("btns_id")
    private val boardingCardItemsRef = reference<List<Div>>("boarding_btns")

    private val templates: TemplateContext<Div> = templates()

    private fun templates(): TemplateContext<Div> {
        val templates: TemplateContext<Div> = templates {
            define(
                "new_icon",
                divImage(
                    width = divFixedSize(82),
                    height = divFixedSize(82),
                    imageUrl = value(URI.create("https://androidtv.s3.yandex.net/feature_boarding/tandem/1/new_3.webp")),
                    preloadRequired = enum(BoolInt.TRUE),
                    transitionIn = divAppearanceSetTransition(
                        items = listOf(
                            divFadeTransition(
                                duration = 400,
                                alpha = 0.0,
                                interpolator = DivAnimationInterpolator.EASE_IN_OUT,
                                startDelay = 250
                            ),
                            divSlideTransition(
                                distance = divDimension(25.0),
                                edge = enum(DivSlideTransition.Edge.BOTTOM),
                                duration = int(400),
                                interpolator = enum(DivAnimationInterpolator.EASE_OUT),
                                startDelay = int(150)
                            )
                        ),
                    ),
                    id = newIconIdRef,
                    alpha = newIconAlphaRef
                )
            )
            define(
                "boarding_button",
                divText(
                    text = boardingButtonTextRef,
                    fontSize = int(14),
                    fontWeight = enum(DivFontWeight.MEDIUM),
                    border = divBorder(cornerRadius = 24),
                    margins = divEdgeInsets(
                        left = 12,
                        top = 20,
                    ),
                    width = divWrapContentSize(),
                    paddings = divEdgeInsets(
                        bottom = 11,
                        top = 11,
                        left = 24,
                        right = 24,
                    ),
                    focus = divFocus(background = listOf(divSolidBackground("#ffffff".color))),
                    focusedTextColor = value("#151517".color),
                    action = divAction(
                        url = boardingButtonActionUrlRef,
                        logId = boardingButtonActionLogIdRef,
                        payload = boardingButtonActionPayloadRef,
                    ),
                    textAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
                    textColor = value("#ffffff".color),
                    background = listOf(divSolidBackground("#333436".color)),
                )
            )
            define(
                "boarding_title",
                divText(
                    fontSize = int(40),
                    fontWeight = enum(DivFontWeight.BOLD),
                    lineHeight = int(45),
                    textColor = value("#ffffff".color),
                    margins = divEdgeInsets(left = 12),
                    ranges = listOf(range(
                        start = 38,
                        end = 48,
                        textColor = "#8126FF".color,
                    )),
                    transitionIn = divAppearanceSetTransition(
                        items = listOf(
                            divFadeTransition(
                                duration = 400,
                                alpha = 0.0,
                                interpolator = DivAnimationInterpolator.EASE_IN_OUT,
                                startDelay = 300
                            ),
                            divSlideTransition(
                                distance = divDimension(30.0),
                                edge = enum(DivSlideTransition.Edge.BOTTOM),
                                duration = int(400),
                                interpolator = enum(DivAnimationInterpolator.EASE_OUT),
                                startDelay = int(250)
                            )
                        ),
                    ),
                    id = boardingTitleIdRef,
                    text = boardingTitleTextRef
                )
            )
            define(
                "boarding_description",
                divText(
                    fontSize = int(16),
                    fontWeight = enum(DivFontWeight.MEDIUM),
                    textColor = value("#ffffff".color),
                    lineHeight = int(22),
                    margins = divEdgeInsets(
                        left = 12,
                        top = 10
                    ),
                    transitionIn = divAppearanceSetTransition(
                        items = listOf(
                            divFadeTransition(
                                duration = 400,
                                alpha = 0.0,
                                interpolator = DivAnimationInterpolator.EASE_IN_OUT,
                                startDelay = 350
                            ),
                            divSlideTransition(
                                distance = divDimension(35.0),
                                edge = enum(DivSlideTransition.Edge.BOTTOM),
                                duration = int(400),
                                interpolator = enum(DivAnimationInterpolator.EASE_OUT),
                                startDelay = int(300)
                            )
                        ),
                    ),
                    id = boardingDescriptionIdRef,
                    text = boardingDescriptionTextRef
                )
            )
            define(
                "boarding_card",
                divContainer(
                    orientation = enum(DivContainer.Orientation.VERTICAL),
                    height = divMatchParentSize(),
                    paddings = divEdgeInsets(
                        top = 81,
                        left = 178,
                    ),
                    background = boardingCardBackgroundRef,
                    items = listOf(
                        template("new_icon"),
                        template("boarding_title"),
                        template("boarding_description"),
                        divContainer(
                            id = boardingCardIdRef,
                            orientation = enum(DivContainer.Orientation.HORIZONTAL),
                            items = boardingCardItemsRef,
                            transitionIn = divAppearanceSetTransition(
                                items = listOf(
                                    divFadeTransition(
                                        duration = 400,
                                        alpha = 0.0,
                                        interpolator = DivAnimationInterpolator.EASE_IN_OUT,
                                        startDelay = 400
                                    ),
                                    divSlideTransition(
                                        distance = divDimension(40.0),
                                        edge = enum(DivSlideTransition.Edge.BOTTOM),
                                        duration = int(400),
                                        interpolator = enum(DivAnimationInterpolator.EASE_OUT),
                                        startDelay = int(400)
                                    )
                                ),
                            ),
                            transitionOut = divFadeTransition(
                                duration = 250,
                                alpha = 0.0,
                                interpolator = DivAnimationInterpolator.EASE_IN,
                            )
                        )
                    )
                )
            )
            define(
                "init_card",
                divText(
                    text = value("Init"),
                    id = value("ss0"),
                    textColor = value("#151517".color),
                    alpha = value(0.0),
                    height = divMatchParentSize(),
                    width = divMatchParentSize(),
                    visibilityAction = divVisibilityAction(
                        logId = "inited",
                        url = URI.create("div-action://set_state?state_id=1/screens/screen1")
                    )
                )
            )
        }
        return templates
    }

    fun getNoTandemDevicePromo(): CardWithTemplates {
        val card = card {
            divData(
                logId = "tandem",
                transitionAnimationSelector = DivTransitionSelector.ANY_CHANGE,
                states = listOf(
                    state(
                        stateId = 1,
                        div = divState(
                            height = divMatchParentSize(),
                            width = divMatchParentSize(),
                            divId = "screens",
                            states = listOf(
                                state(
                                    stateId = "init",
                                    div = template(type = "init_card")
                                ),
                                state(
                                    stateId = "screen1",
                                    div = template(
                                        "boarding_card",
                                        override("id", "ss1"),
                                        resolve(newIconIdRef, "icon1"),
                                        resolve(boardingTitleIdRef, "title1"),
                                        resolve(boardingDescriptionIdRef, "description1"),
                                        resolve(boardingCardIdRef, "btns1"),
                                        resolve(newIconAlphaRef, 1.0),
                                        resolve(boardingTitleTextRef, "Телевизором можно\nуправлять голосом —\nбез пульта"),
                                        resolve(boardingDescriptionTextRef, "Подключите колонку Яндекса и ТВ к одной сети,\nнастройте тандем — и говорите Алисе, что делать.\nЕсли у вас ещё нет колонки, закажите её на Маркете."),
                                        resolve(boardingCardItemsRef, listOf(template(
                                            "boarding_button",
                                            resolve(boardingButtonTextRef, "Понятно"),
                                            resolve(boardingButtonActionUrlRef, URI.create("div-action://finish")),
                                            resolve(boardingButtonActionLogIdRef, "complete"),
                                        ))),
                                        override("transition_in", divFadeTransition(
                                            duration = 400,
                                            alpha = 0.0,
                                            interpolator = DivAnimationInterpolator.EASE_OUT
                                        )),
                                        resolve(boardingCardBackgroundRef, listOf(
                                            divSolidBackground("#151517".color),
                                            divImageBackground(
                                                imageUrl = value(URI.create("https://quasar.s3.yandex.net/feature_boarding/tandem/3/tandem_background.webp")),
                                                preloadRequired = enum(BoolInt.TRUE)
                                            )
                                        ))
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
