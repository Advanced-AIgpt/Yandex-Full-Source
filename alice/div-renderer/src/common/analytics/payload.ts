/* Schema follows https://wiki.yandex-team.ru/users/mysov-gr/centaurlogging/client-side-log-specification-v2/#eventvaluev.2 */

import { AnalyticsContext, MediaType } from './context';

export interface IActionAnalyticsPayload {
    /* card_id, uuid карточки/элемента карточки, генерируется в момент формирования ответа на беке */
    instance_id?: (string | null),
    /* uuid фулскриновой карточки родителя */
    screen_instance_id?: (string | null),
    /* + */
    screen_title?: (string | null),
    /* req_id - id исходного запроса */
    req_id?: (string | null),
    /* Render-element-based keys */
    /* card_id, uuid карточки контейнера */
    parent_product_item_instance_id?: (string | null),
    /* Обязательно. Галерея/Карточка в галерее/Кнопка. Вероятно log_id */
    element_type?: (string | null),
    element_meta_info?: (IActionAnalyticsMetaInfoPayload | null)
}

export interface IActionAnalyticsMetaInfoPayload {
    title?: (string | null),
    subtitle?: (string | null),
    description?: (string | null),
    /* None/Text/Picture/Music/Video */
    media_type?: (string | null),
    /* Type-dependent */
    media_meta?: (IActionAnalyticsMediaMetaPayload | null)
}

export interface IActionAnalyticsMediaMetaPayload {
    /* Необязательно. Если есть лого/обложка. */
    picture_url?: (string | null),
    /* Необязательно. Если под карточкой есть конкретный объект. */
    object_id?: (string | null),
    /* Необязательно. Для музыки например плейлист/трек/радио. */
    object_type?: (string | null),
    /* Необязательно. Если есть ссылка на объект. */
    object_url?: (string | null),
}

// TODO: generalize
export function FormActionAnalyticsPayload(
    analyticsContext: AnalyticsContext,
): IActionAnalyticsPayload {
    return {
        instance_id: analyticsContext.element?.elementId,
        screen_instance_id: analyticsContext.screenInstanceId,
        screen_title: analyticsContext.screenTitle,
        req_id: analyticsContext.reqId,
        // todo
        parent_product_item_instance_id: null,
        element_type: analyticsContext?.element?.elementType,
        element_meta_info: {
            title: analyticsContext?.element?.elementType,
            description: analyticsContext?.element?.description,
            media_type: analyticsContext?.mediaMeta?.mediaType ?? MediaType.None,
            ...(analyticsContext?.mediaMeta && {
                media_meta: {
                    picture_url: analyticsContext?.mediaMeta?.pictureUrl,
                    object_id: analyticsContext?.mediaMeta?.objectId,
                    object_type: analyticsContext?.mediaMeta?.objectType,
                    object_url: analyticsContext?.mediaMeta?.objectUrl,
                },
            }),
        },
    };
}
