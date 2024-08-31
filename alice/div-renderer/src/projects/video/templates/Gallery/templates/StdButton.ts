import { SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { GetOrDefault } from '../utils';

export function StdButton(cardData: NAlice.NData.TGalleryData.ITCard, buttonText: string, buttonActionUrl: string,
    buttonLogId: string) {
    return new TextBlock({
        text: buttonText,
        font_size: 14,
        font_weight: 'medium',
        border: {
            corner_radius: 24,
            has_shadow: 0,
        },
        margins: {
            bottom: 5,
            left: 5,
            right: 5,
            top: 5,
        },
        width: new WrapContentSize(),
        paddings: {
            bottom: 11,
            top: 11,
            left: 16,
            right: 16,
        },
        action: {
            url: buttonActionUrl,
            log_id: buttonLogId,
            payload: {
                title: GetOrDefault(cardData?.TitleText, ''),
                content_id: GetOrDefault(cardData?.ContentId, ''),
                content_type: GetOrDefault(cardData?.ContentType, ''),
                onto_id: GetOrDefault(cardData?.OntoId, ''),
                on_vh_content_card_opened_event: GetOrDefault(cardData?.OnVhContentCardOpenedEvent, {}),
                general_analytics_info_payload: GetOrDefault(cardData?.CarouselItemGeneralAnalyticsInfoPayload, {}),
            },
        },
        text_alignment_horizontal: 'center',
        text_color: '#ffffff',
        background: [
            new SolidBackground({
                color: '#333436',
            }),
        ],
        extensions: [
            {
                id: 'request_focus',
            },
        ],
        focus: {
            background: [
                new SolidBackground({
                    color: '#ffffff',
                }),
            ],
        },
        focused_text_color: '#151517',
    });
}
