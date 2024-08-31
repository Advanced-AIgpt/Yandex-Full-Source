package ru.yandex.alice.paskill.dialogovo.service.appmetrica

object MetricaEventConstants {
    val utteranceMetricaEvent = MetricaEvent("utterance")
    val buttonPressedMetricaEvent = MetricaEvent("button_pressed")
    val endSessionMetricaEvent = MetricaEvent("end_session")
    val canvasCallbackMetricaEvent = MetricaEvent("canvas_callback")
    val showPullMetricaEvent = MetricaEvent("show_pull")
    val accountLinkingCompleteMetricaEvent = MetricaEvent("account_linking_complete")
    val userAgreementsAcceptedMetricaEvent = MetricaEvent("user_agreements_accepted")
    val userAgreementsRejectedMetricaEvent = MetricaEvent("user_agreements_rejected")
    val skillProductActivatedMetricaEvent = MetricaEvent("skill_product_activated")
    val skillProductActivationFailedMetricaEvent = MetricaEvent("skill_product_activation_failed")
    val skillGeolocationAllowedMetricaEvent = MetricaEvent("skill_geolocation_allowed")
    val skillGeolocationRejectedMetricaEvent = MetricaEvent("skill_geolocation_rejected")
    val skillPurchaseCompleteMetricaEvent = MetricaEvent("skill_purchase_complete")
    val skillPurchaseConfirmationMetricaEvent = MetricaEvent("skill_purchase_confirmation")
    val newsReadMetricaEvent = MetricaEvent("news_read")
    val widgetGalleryMetricaEvent = MetricaEvent("widget_gallery_shown")
    val teasersMetricaEvent = MetricaEvent("teasers_shown")
}
