package ru.yandex.alice.divkttemplates.tvmain

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.yandex.div.dsl.*
import com.yandex.div.dsl.context.*
import com.yandex.div.dsl.model.*
import com.yandex.div.dsl.type.BoolInt
import com.yandex.div.dsl.type.Color
import com.yandex.div.dsl.type.color
import ru.yandex.alice.library.protobufutils.JacksonToProtoConverter
import ru.yandex.alice.protos.data.scenario.video.Gallery.TGalleryData
import ru.yandex.alice.protos.data.video.VideoProto.TAvatarMdsImage
import java.math.RoundingMode
import java.net.URI
import java.text.DecimalFormat
import kotlin.math.floor

object TvMainTemplate {
    private val gradientButtonTextRef = reference<String>("button_text")
    private val gradientButtonActionUrlRef = reference<URI>("button_action")
    private val gradientButtonActionLogIdRef = reference<String>("button_log_id")
    private val gradientButtonActionPayloadRef = reference<Map<String, Any>>("action_payload")

    private val stdButtonTextRef = reference<String>("std_button_text")
    private val stdButtonActionUrlRef = reference<URI>("std_button_action_uri")
    private val stdButtonActionLogIdRef = reference<String>("std_button_action_log_id")
    private val stdButtonActionPayloadRef = reference<Map<String, Any>>("std_button_acrion_payload")

    private val titleImageUrlRef = reference<URI>("logo_url")
    private val titleImageHasAlphaRef = reference<Double>("has_logo")

    private val titleIdRef = reference<String>("title_id")
    private val titleTextRef = reference<String>("title_text")
    private val titleTextSizeRef = reference<Int>("title_text_size")

    private val legalLogoUrlRef = reference<URI>("legal_logo_url")
    private val hasLegalLogoAlphaRef = reference<Double>("has_legal_logo")

    private val descriptionIdRef = reference<String>("description")
    private val descriptionTextRef = reference<String>("description_text")

    private val metaIdRef = reference<String>("meta_id")
    private val metaTextRef = reference<String>("meta_text")

    private val subscriptionIdRef = reference<String>("subscription_id")
    private val subscriptionTextRef = reference<String>("subscription_text")

    private val ratingTextRef = reference<String>("rating_text")
    private val ratingTextColorRef = reference<Color>("rating_color")
    private val ratingIdRef = reference<String>("rating_id")

    private val thumbnailImageUrlRef = reference<URI>("image_url")

    private val promoItemItemsBtnsRef = reference<List<Div>>("btns")

    private val converter = JacksonToProtoConverter(jacksonObjectMapper().disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES))

    private val templates: TemplateContext<Div> = templates()

    fun renderGallery(galleryData: TGalleryData): CardWithTemplates {

        val card = card {
            divData(
                logId = "video.top_div_gallery",
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
                                    div = divContainer(
                                        id = "ss1",
                                        background = listOf(divSolidBackground(color = "#151517".color)),
                                        orientation = DivContainer.Orientation.OVERLAP,
                                        height = divMatchParentSize(),
                                        visibilityAction = divVisibilityAction(
                                            logId = "carousel_show",
                                            logLimit = 0,
                                            payload = mapOf(
                                                "carousel_id" to galleryData.galleryId,
                                                "title" to galleryData.galleryTitle,
                                                "place" to galleryData.galleryParentScreen,
                                                "position" to galleryData.galleryPosition,
                                            ),
                                            url = URI.create("metrics://carouselShow"),
                                            visibilityDuration = 1000
                                        ),
                                        items = listOf(
                                            divState(
                                                divId = "thumbnails",
                                                height = divMatchParentSize(),
                                                states = galleryData.cardsList.mapIndexed { index, cardData ->
                                                    state(
                                                        stateId = "thumbnail${index}",
                                                        div = fillThumbnailTemplate(cardData)
                                                    )
                                                }
                                            ),
                                            divState(
                                                divId = "cards",
                                                height = divMatchParentSize(),
                                                states = galleryData.cardsList.mapIndexed { index, cardData ->
                                                    state(
                                                        stateId = "card${index}",
                                                        div = fillGalleryTemplate(cardData, galleryData, index)
                                                    )
                                                },
                                                background = listOf(
                                                    divGradientBackground(
                                                        angle = int(0),
                                                        colors = value(
                                                            listOf(
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#f2151517".color,
                                                                "#e0151517".color,
                                                                "#cc151517".color,
                                                                "#b5151517".color,
                                                                "#9b151517".color,
                                                                "#7f151517".color,
                                                                "#64151517".color,
                                                                "#4a151517".color,
                                                                "#33151517".color,
                                                                "#1f151517".color,
                                                                "#0d151517".color,
                                                                "#05151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                            )
                                                        )
                                                    ),
                                                    divGradientBackground(
                                                        angle = int(90),
                                                        colors = value(
                                                            listOf(
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#ff151517".color,
                                                                "#bf151517".color,
                                                                "#7f151517".color,
                                                                "#3f151517".color,
                                                                "#1f151517".color,
                                                                "#0f151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                                "#00151517".color,
                                                            )
                                                        )
                                                    )
                                                )
                                            ),
                                            divCustom(
                                                customType = "indicator",
                                                customProps = mapOf(
                                                    "count" to galleryData.cardsCount,
                                                    "size" to 9,
                                                    "space" to 4,
                                                    "color" to "#60ffffff".color,
                                                    "selected_color" to "#ffffff".color,
                                                    "visible_count" to 4,
                                                ),
                                                alignmentHorizontal = DivAlignmentHorizontal.RIGHT,
                                                alignmentVertical = DivAlignmentVertical.BOTTOM,
                                                width = divWrapContentSize(),
                                                margins = divEdgeInsets(
                                                    right = 36,
                                                    bottom = 244
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

    private fun TemplateContext<Div>.textTransitionIn() = divFadeTransition(
        duration = int(300),
        alpha = value(0.0),
        interpolator = enum(DivAnimationInterpolator.EASE_IN_OUT)
    )

    private fun templates(): TemplateContext<Div> {
        val templates: TemplateContext<Div> = templates {
            define(
                "gradient_button",
                gradientButton()
            )
            define(
                "std_button",
                stdButton(),
            )
            define(
                "title_image",
                divImage(
                    id = value("logo"),
                    alignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                    contentAlignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                    alignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                    contentAlignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                    height = divMatchParentSize(),
                    scale = enum(DivImageScale.FIT),
                    margins = divEdgeInsets(
                        left = int(20),
                        right = int(20),
                        top = int(23),
                    ),
                    extensions = listOf(
                        divExtension(
                            id = value("transition"),
                            params = value(
                                mapOf<String, Any>(
                                    "name" to "logo",
                                    "target" to "logo"
                                )
                            )
                        )
                    ),
                    appearanceAnimation = divFadeTransition(
                        duration = int(200),
                        alpha = value(0.1),
                        interpolator = enum(DivAnimationInterpolator.EASE_IN_OUT)
                    ),
                    imageUrl = titleImageUrlRef,
                    alpha = titleImageHasAlphaRef,
                )
            )
            define(
                "title",
                divText(
                    transitionIn = textTransitionIn(),
                    transitionTriggers = value(DivTransitionTrigger.values().toList()),
                    fontSize = titleTextSizeRef,
                    fontWeight = enum(DivFontWeight.BOLD),
                    lineHeight = int(40),
                    textColor = value("#ffffff".color),
                    alignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                    alignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                    textAlignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                    textAlignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                    truncate = enum(DivText.Truncate.END),
                    maxLines = int(2),
                    height = divMatchParentSize(),
                    margins = divEdgeInsets(
                        left = int(20),
                    ),
                    id = titleIdRef,
                    text = titleTextRef
                )
            )
            define(
                "legal_logo",
                divImage(
                    id = value("legal_logo"),
                    alignmentHorizontal = enum(DivAlignmentHorizontal.RIGHT),
                    contentAlignmentHorizontal = enum(DivAlignmentHorizontal.RIGHT),
                    alignmentVertical = enum(DivAlignmentVertical.TOP),
                    contentAlignmentVertical = enum(DivAlignmentVertical.TOP),
                    width = divWrapContentSize(),
                    height = divFixedSize(value = int(60)),
                    scale = enum(DivImageScale.FIT),
                    margins = divEdgeInsets(
                        right = int(30),
                        top = int(52),
                    ),
                    appearanceAnimation = divFadeTransition(
                        duration = int(300),
                        alpha = value(0.1),
                        startDelay = int(100),
                        interpolator = enum(DivAnimationInterpolator.EASE_IN_OUT)
                    ),
                    imageUrl = legalLogoUrlRef,
                    alpha = hasLegalLogoAlphaRef
                )
            )
            define(
                "description",
                divText(
                    transitionIn = textTransitionIn(),
                    transitionTriggers = value(DivTransitionTrigger.values().toList()),
                    fontSize = int(16),
                    fontWeight = enum(DivFontWeight.REGULAR),
                    lineHeight = int(20),
                    textColor = value("#ffffff".color),
                    maxLines = value(3),
                    truncate = enum(DivText.Truncate.END),
                    width = divFixedSize(value = int(400)),
                    height = divFixedSize(value = int(70)),
                    margins = divEdgeInsets(
                        left = int(20),
                        right = int(20),
                        top = int(3),
                    ),
                    id = descriptionIdRef,
                    text = descriptionTextRef
                )
            )
            define(
                "meta",
                divText(
                    transitionIn = textTransitionIn(),
                    transitionTriggers = value(DivTransitionTrigger.values().toList()),
                    fontSize = int(14),
                    fontWeight = enum(DivFontWeight.MEDIUM),
                    textColor = value("#b3b3b3".color),
                    maxLines = value(1),
                    truncate = enum(DivText.Truncate.END),
                    width = divFixedSize(value = int(400)),
                    text = metaTextRef,
                    id = metaIdRef
                )
            )
            define(
                "subscription",
                divText(
                    transitionIn = textTransitionIn(),
                    transitionTriggers = value(DivTransitionTrigger.values().toList()),
                    fontSize = int(14),
                    fontWeight = enum(DivFontWeight.REGULAR),
                    textColor = value("#b3b3b3".color),
                    maxLines = value(1),
                    truncate = enum(DivText.Truncate.END),
                    width = divFixedSize(value = int(400)),
                    margins = divEdgeInsets(
                        left = int(20),
                        right = int(20),
                        top = int(18),
                    ),
                    id = subscriptionIdRef,
                    text = subscriptionTextRef
                )
            )
            define(
                "rating",
                divText(
                    transitionIn = textTransitionIn(),
                    transitionTriggers = value(DivTransitionTrigger.values().toList()),
                    fontSize = int(14),
                    fontWeight = enum(DivFontWeight.MEDIUM),
                    maxLines = value(1),
                    text = ratingTextRef,
                    id = ratingIdRef,
                    textColor = ratingTextColorRef
                )
            )
            define(
                "thumbnail",
                divImage(
                    id = value("thumbnail"),
                    width = divFixedSize(value = int(700)),
                    height = divFixedSize(value = int(400)),
                    extensions = listOf(
                        divExtension(
                            id = value("transition"),
                            params = value(
                                mapOf<String, Any>(
                                    "name" to "thumbnail",
                                    "target" to "thumbnail"
                                )
                            )
                        )
                    ),
                    alignmentHorizontal = enum(DivAlignmentHorizontal.RIGHT),
                    alignmentVertical = enum(DivAlignmentVertical.TOP),
                    appearanceAnimation = divFadeTransition(
                        duration = int(200),
                        alpha = value(0.1),
                        interpolator = enum(DivAnimationInterpolator.EASE_IN_OUT)
                    ),
                    imageUrl = thumbnailImageUrlRef,
                )
            )
            define(
                "promo_item",
                divContainer(
                    id = value("item"),
                    orientation = enum(DivContainer.Orientation.OVERLAP),
                    height = divMatchParentSize(),
                    items = listOf(
                        divContainer(
                            contentAlignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                            contentAlignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                            height = divMatchParentSize(),
                            items = listOf(
                                divContainer(
                                    orientation = enum(DivContainer.Orientation.OVERLAP),
                                    contentAlignmentHorizontal = enum(DivAlignmentHorizontal.LEFT),
                                    contentAlignmentVertical = enum(DivAlignmentVertical.BOTTOM),
                                    height = divMatchParentSize(),
                                    width = divFixedSize(value = int(400)),
                                    items = listOf(
                                        template("title"),
                                        template("title_image"),
                                    )
                                ),
                                divContainer(
                                    orientation = enum(DivContainer.Orientation.HORIZONTAL),
                                    width = divWrapContentSize(),
                                    margins = divEdgeInsets(
                                        left = int(20),
                                        right = int(20),
                                        top = int(12),
                                    ),
                                    items = listOf(
                                        template("rating"),
                                        template("meta")
                                    )
                                ),
                                template("description"),
                                template("subscription"),
                                divContainer(
                                    orientation = enum(DivContainer.Orientation.HORIZONTAL),
                                    margins = divEdgeInsets(
                                        top = int(7),
                                        bottom = int(232),
                                        left = int(15),
                                        right = int(15),
                                    ),
                                    items = promoItemItemsBtnsRef
                                )
                            )
                        ),
                        template("legal_logo")
                    )
                )
            )
            define(
                "init_card",
                divText(
                    text = value("Init"),
                    id = value("ss0"),
                    textColor = value("#170d1f".color),
                    alpha = value(0.0),
                    height = divMatchParentSize(),
                    width = divMatchParentSize(),
                    visibilityAction = divVisibilityAction(
                        logId = value("inited"),
                        url = value(URI.create("div-action://set_state?state_id=1/screens/screen1")),
                        visibilityDuration = value(0)
                    )
                )
            )
        }
        return templates
    }

    private fun CardContext.fillThumbnailTemplate(cardData: TGalleryData.TCard) = template(
        "thumbnail",
        resolve(thumbnailImageUrlRef, chooseAvatarsMdsImage(cardData.thumbnailUrls)),
    )

    private fun CardContext.fillGalleryTemplate(cardData: TGalleryData.TCard, galleryData: TGalleryData, index: Int) = template(
        "promo_item",
        resolve(titleImageUrlRef, chooseAvatarsMdsImage(cardData.logoUrls)),
        resolve(titleImageHasAlphaRef, if (cardData.hasLogoUrls()) 1.0 else 0.0),
        resolve(descriptionIdRef, cardData.descriptionText),
        resolve(legalLogoUrlRef, chooseAvatarsMdsImage(cardData.legalLogoUrls)),
        resolve(hasLegalLogoAlphaRef, if (cardData.hasLegalLogoUrls()) 1.0 else 0.0),
        resolve(metaTextRef, getMetaText(cardData)),
        resolve(subscriptionTextRef, " "),
        resolve(ratingTextRef, cardData.rating.let {
            val df = DecimalFormat("#.#")
            df.roundingMode = RoundingMode.HALF_UP
            return@let df.format(it).toDouble()
        }.toString()),
        resolve(ratingTextColorRef, getRatingColor(cardData.rating)),
        resolve(descriptionTextRef, cardData.descriptionText),
        resolve(titleTextRef, if (cardData.hasLogoUrls()) "" else cardData.titleText),
        resolve(titleTextSizeRef, if (cardData.titleText.length < 20) 50 else 36),
        resolve(titleIdRef, "title${index}"),
        resolve(metaIdRef, "meta${index}"),
        resolve(ratingIdRef, "rating${index}"),
        resolve(subscriptionIdRef, "subscription${index}"),
        override("visibility_actions", listOf(
            divVisibilityAction(
                logId = "show_thumbnail",
                logLimit = 0,
                url = URI.create("div-action://set_state?state_id=1/screens/screen1/thumbnails/thumbnail$index"),
                visibilityDuration = 0,
            ),
            divVisibilityAction(
                logId = "card_show",
                logLimit = 0,
                url = URI.create("metrics://cardShow"),
                visibilityDuration = 1000,
                payload = mapOf(
                    "carousel" to mapOf<String, Any>(
                        "carousel_id" to galleryData.galleryId,
                        "title" to galleryData.galleryTitle,
                        "place" to galleryData.galleryParentScreen,
                        "position" to galleryData.galleryPosition
                    ),
                    "content-item" to mapOf<String, Any>(
                        "content_id" to cardData.contentId,
                        "content_type" to cardData.contentType,
                        "req_info" to mapOf<String, Any>(
                            "reqid" to galleryData.requestId,
                            "apphost-reqid" to galleryData.apphostRequestId,
                        )
                    ),
                    "position" to index
                )
            )
        )),
        resolve(promoItemItemsBtnsRef, listOf(template(
            "std_button",
            resolve(stdButtonTextRef, "Смотреть"),
            resolve(stdButtonActionUrlRef, URI.create("router://openDetails")),
            resolve(stdButtonActionLogIdRef, "open_item"),
            resolve(stdButtonActionPayloadRef, mapOf<String, Any>(
                "title" to cardData.titleText,
                "content_id" to cardData.contentId,
                "content_type" to cardData.contentType,
                "duration_s" to cardData.durationSeconds,
                "ya_plus" to cardData.subscriptionTypesList,
                "licenses" to getLicenses(cardData.requiredLicenseType),
                "genres" to cardData.genresList.joinToString(", "),
                "onto_id" to cardData.ontoId,
                "on_vh_content_card_opened_event" to converter.structToMap(cardData.onVhContentCardOpenedEvent),
                "general_analytics_info_payload" to converter.structToMap(cardData.carouselItemGeneralAnalyticsInfoPayload),
            ))
        )))
    )

    private fun getLicenses(licenseType: TGalleryData.TCard.LicenseType): List<String> =
        when (licenseType) {
            TGalleryData.TCard.LicenseType.UNRECOGNIZED -> emptyList()
            else -> listOf(licenseType.name)
        }

    private fun chooseAvatarsMdsImage(image: TAvatarMdsImage?): URI {
        if (image == null || image == TAvatarMdsImage.getDefaultInstance() || image.baseUrl?.length == 0 || image.sizesCount == 0) {
            return URI.create("")
        }
        val baseUrl = image.baseUrl
        if (baseUrl == null || baseUrl.isEmpty()) {
            return URI.create("")
        }
        val sizes = image.sizesList
        if (sizes == null || sizes.size == 0 || sizes[0].isEmpty()) {
            return URI.create(baseUrl.toString() + "orig")
        }

        return URI.create(baseUrl.toString() + sizes[0])
    }

    private fun getMetaText(cardData: TGalleryData.TCard): String {
        return listOf(
            cardData.genresList?.take(2)?.joinToString(", "),
            cardData.releaseYear.takeIf { it != 0 }.toString(),
            cardData.countries?.split(",")?.take(2)?.joinToString(", "),
            getDuration(cardData.durationSeconds),
            getAgeLimit(cardData.ageLimit)
        ).filterNot{it.isNullOrEmpty()}.joinToString(separator = " • ", prefix = " • ")
    }

    private fun getDuration(duration: Long?): String {
        var res = ""
        if (duration != null) {
            val hrs = floor((duration / 3600).toDouble()).toInt()
            val min = floor(((duration % 3600) / 60).toDouble()).toInt()
            if (hrs > 0) {
                res += "${hrs} ч"
            }
            if (min > 0) {
                res += " ${min} мин"
            }
        }
        return res
    }

    private fun getAgeLimit(ageLimit: Int?): String {
        if (ageLimit != null) {
            return "$ageLimit+"
        }
        return ""
    }

    private fun getRatingColor(rating: Float?): Color {
        if (rating == null || rating < 2) {
            return "#727272".color
        }
        if (rating < 4) {
            return "#85855d".color
        }
        if (rating < 6) {
            return "#91a449".color
        }
        if (rating < 8) {
            return "#89c939".color
        }
        return "#32ba43".color
    }

    private fun TemplateContext<Div>.stdButton() = divText(
        text = stdButtonTextRef,
        fontSize = int(14),
        fontWeight = enum(DivFontWeight.MEDIUM),
        border = divBorder(
            cornerRadius = int(24),
            hasShadow = enum(BoolInt.FALSE),
        ),
        margins = divEdgeInsets(
            bottom = int(5),
            left = int(5),
            right = int(5),
            top = int(5),
        ),
        width = divWrapContentSize(),
        paddings = divEdgeInsets(
            bottom = int(11),
            top = int(11),
            left = int(16),
            right = int(16),
        ),
        action = divAction(
            url = stdButtonActionUrlRef,
            logId = stdButtonActionLogIdRef,
            payload = stdButtonActionPayloadRef,
        ),
        textAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
        textColor = value("#ffffff".color),
        background = listOf(divSolidBackground(color = value("#333436".color))),
        extensions = listOf(divExtension(id = value("request_focus"))),
        focus = divFocus(background = listOf(divSolidBackground(color = value("#ffffff".color)))),
        focusedTextColor = value("#151517".color),
    )

    private fun TemplateContext<Div>.gradientButton() = divText(
        text = gradientButtonTextRef,
        fontSize = int(14),
        fontWeight = enum(DivFontWeight.MEDIUM),
        border = divBorder(
            cornerRadius = int(24),
            hasShadow = enum(BoolInt.FALSE),
        ),
        margins = divEdgeInsets(
            bottom = int(5),
            left = int(5),
            right = int(5),
            top = int(5),
        ),
        width = divWrapContentSize(),
        paddings = divEdgeInsets(
            bottom = int(11),
            top = int(11),
            left = int(16),
            right = int(16),
        ),
        action = divAction(
            url = gradientButtonActionUrlRef,
            logId = gradientButtonActionLogIdRef,
            payload = gradientButtonActionPayloadRef,
        ),
        textAlignmentHorizontal = enum(DivAlignmentHorizontal.CENTER),
        textColor = value("#ffffff".color),
        background = listOf(divSolidBackground(color = value("#333436".color))),
        extensions = listOf(divExtension(id = value("request_focus"))),
        focus = divFocus(
            background = listOf(
                divGradientBackground(
                    angle = int(190),
                    colors = value(
                        listOf(
                            "#F19743".color,
                            "#AF44C4".color,
                            "#5259DC".color,
                            "#505ADD".color
                        )
                    )
                )
            )
        ),
        focusedTextColor = value("#ffffff".color),
    )
}
