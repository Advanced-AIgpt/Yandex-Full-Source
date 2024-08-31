import {
    WrapContentSize,
    MatchParentSize,
    TextBlock,
    Div, DivCustomBlock, TemplateBlock,
} from 'divcard2';
import { NAlice } from '../../../../../protos';
import { ChooseAvatarsMdsImage, GetOrDefault } from '../utils';
import { StdButton } from './StdButton';
import { Long } from 'long';

export function InitCardTemplate(): Div {
    return new TextBlock({
        text: 'Init',
        id: 'ss0',
        text_color: '#170d1f',
        alpha: 0.0,
        height: new MatchParentSize(),
        width: new MatchParentSize(),
        visibility_action: {
            log_id: 'inited',
            url: 'div-action://set_state?state_id=1/screens/screen1',
        },
    });
}

export function GetDocumentTemplate(
    cardData: NAlice.NData.TGalleryData.ITCard,
    galleryData: NAlice.NData.ITGalleryData,
    index: number): Div {
    return new TemplateBlock('promo_item', {
        id: `item${index}`,
        logo_url: ChooseAvatarsMdsImage(GetOrDefault(cardData.LogoUrls, {})),
        has_logo: cardData.LogoUrls ? 1 : 0,
        thumbnail_url: ChooseAvatarsMdsImage(GetOrDefault(cardData.ThumbnailUrls, {})),
        description: GetOrDefault(cardData.DescriptionText, ''),
        legal_logo_url: ChooseAvatarsMdsImage(GetOrDefault(cardData.LegalLogoUrls, {})),
        has_legal_logo: cardData.LegalLogoUrls ? 1 : 0,
        meta_text: GetMetaText(cardData),
        subscription_text: ' ',
        rating_text: GetOrDefault(cardData.Rating?.toFixed(1).toString(), ''),
        rating_color: GetRatingColor(cardData.Rating),
        description_text: cardData.DescriptionText,
        title_text: cardData.TitleText,
        thumbnail_id: `thumbnail${index}`,
        title_id: `title${index}`,
        legal_logo_id: `legallogo${index}`,
        meta_id: `meta${index}`,
        rating_id: `rating${index}`,
        subscription_id: `subscription${index}`,
        visibility_action: {
            log_id: 'card_show',
            log_limit: 0,
            url: 'metrics://cardShow',
            visibility_duration: 1000,
            payload: {
                carousel: {
                    carousel_id: galleryData.GalleryId,
                    title: galleryData.GalleryTitle,
                    place: galleryData.GalleryParentScreen,
                    position: galleryData.GalleryPosition,
                },
                content_item: {
                    content_id: cardData.ContentId,
                    content_type: cardData.ContentType,
                    req_info: {
                        reqid: galleryData.RequestId,
                        'apphost-reqid': galleryData.ApphostRequestId,
                    },
                },
                position: index,
            },
        },
        btns: [
            StdButton(cardData, 'Смотреть', 'router://openDetails', 'open_item'),
        ],
    });
}

export function GetIndicatorTemplate(): Div {
    return new DivCustomBlock({
        custom_type: 'indicator',
        custom_props: {
            count: 10,
            size: 9,
            space: 4,
            color: '#333333',
            selected_color: '#ffffff',
            visible_count: 4,
        },
        alignment_horizontal: 'right',
        alignment_vertical: 'bottom',
        width: new WrapContentSize(),
        margins: {
            right: 36,
            bottom: 244,
        },
    });
}

function GetRatingColor(rating: (number|null|undefined)): string {
    if (rating == null || rating < 2) {
        return '#727272';
    }
    if (rating < 4) {
        return '#85855d';
    }
    if (rating < 6) {
        return '#91a449';
    }
    if (rating < 8) {
        return '#89c939';
    }
    return '#32ba43';
}

function GetDuration(duration: (number|Long|null|undefined)): string {
    let res = '';
    if (duration) {
        const hrs = Math.floor(Number(duration) / 3600);
        const min = Math.floor((Number(duration) % 3600) / 60);
        if (hrs > 0) {
            res += `${hrs} ч`;
        }
        if (min > 0) {
            res += ` ${min} мин`;
        }
    }
    return res;
}

function GetAgeLimit(ageLimit: number|null|undefined): string {
    if (ageLimit !== null && typeof ageLimit !== 'undefined') {
        return ageLimit.toFixed(0) + '+';
    }
    return '';
}

function GetMetaText(cardData: NAlice.NData.TGalleryData.ITCard): string {
    return ' • ' + [
        cardData.Genres?.slice(0, 2).join(', '),
        cardData.ReleaseYear,
        cardData.Countries?.split(',').slice(0, 2).join(', '),
        GetDuration(cardData.DurationSeconds),
        GetAgeLimit(cardData.AgeLimit),
    ].filter(Boolean).join(' • ');
}
