package ru.yandex.alice.social.sharing.document

import com.google.protobuf.Message
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.proto.WebPageProto

private fun String?.nullIfEmpty(): String? = if (this.isNullOrEmpty()) null else this
private fun ByteArray?.nullIfEmpty(): ByteArray? = if (this == null || this.isEmpty()) null else this

data class Document(
    val id: String,
    val page: WebPageProto.TScenarioSharePage,
) {
    val opengraphTitle: String?
        get() = page.socialNetworkMarkup.title.nullIfEmpty()
    val opengraphType: String?
        get() = page.socialNetworkMarkup.type.nullIfEmpty()
    val opengraphUrl: String?
        get() = page.socialNetworkMarkup.url.nullIfEmpty()
    val opengraphDescription: String?
        get() = page.socialNetworkMarkup.description.nullIfEmpty()
    val opengraphImageUrl: String?
        get() = page.socialNetworkMarkup.image.url.nullIfEmpty()
    val opengraphImageType: String?
        get() = page.socialNetworkMarkup.image.type.nullIfEmpty()
    val opengraphImageWidth: Int
        get() = page.socialNetworkMarkup.image.width
    val opengraphImageHeight: Int
        get() = page.socialNetworkMarkup.image.height
    val opengraphImageAlt: String?
        get() = page.socialNetworkMarkup.image.alt.nullIfEmpty()
    val requiredFeatures: List<String>
        get() = page.requiredFeaturesList
    val templateType: Template
        get() = when {
            page.hasDialogWithImage() -> Template.DIALOG_WITH_IMAGE
            page.hasExternalSkill() -> Template.EXTERNAL_SKILL
            page.hasFairyTaleTemplate() -> Template.GENERATIVE_FAIRY_TALE
            else -> Template.NONE
        }
    val templateData: Message?
        get() = when (templateType) {
            Template.DIALOG_WITH_IMAGE -> page.dialogWithImage
            Template.EXTERNAL_SKILL -> page.externalSkill
            Template.GENERATIVE_FAIRY_TALE -> page.fairyTaleTemplate
            Template.NONE -> null
        }
    val typedSemanticFrame: ByteArray?
        get() = if (page.frame != FrameProto.TTypedSemanticFrame.getDefaultInstance()) {
            page.frame.toByteArray()
        } else {
            null
        }
}
