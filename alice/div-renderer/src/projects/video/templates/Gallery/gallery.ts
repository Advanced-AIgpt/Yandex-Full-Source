import {
    TemplateCard,
    Templates,
    ContainerBlock,
    MatchParentSize,
    DivStateBlock,
    SolidBackground,
} from 'divcard2';
import {
    InitCardTemplate,
    GetDocumentTemplate,
    GetIndicatorTemplate,
} from './templates/GalleryTemplates';
import { NAlice } from '../../../../protos';
import { AnimatedText } from './templates/AnimatedText';
import { BaseButton } from './templates/BaseButton';
import { GradientButton } from './templates/GradientButton';
import { StdButton } from './templates/StdButton';
import { TitleImage } from './templates/TitleImage';
import { Title } from './templates/Title';
import { LegalLogo } from './templates/LegalLogo';
import { Description } from './templates/Description';
import { Meta } from './templates/Meta';
import { Subscription } from './templates/Subscription';
import { Rating } from './templates/Rating';
import { Thumbnail } from './templates/Thumbnail';
import { PromoItem } from './templates/PromoItem';
import { ErrorCard } from './templates/ErrorCard';
import { InitCard } from './templates/InitCard';

const templates = {
    animated_text: AnimatedText(),
    base_button: BaseButton(),
    gradient_button: GradientButton(),
    std_button: StdButton({}, '', '', ''),
    title_image: TitleImage(),
    title: Title(),
    legal_logo: LegalLogo(),
    description: Description(),
    meta: Meta(),
    subscription: Subscription(),
    rating: Rating(),
    thumbnail: Thumbnail(),
    promo_item: PromoItem(),
    error_card: ErrorCard(),
    init_card: InitCard(),
};

export const galleryRender = (data: NAlice.NData.ITGalleryData) => {
    if (data.Cards == null) {
        throw new Error('Cards must always be specified');
    }
    return new TemplateCard(new Templates(templates), {
        log_id: 'video.top_div_gallery',
        states: [
            {
                state_id: 1,
                div: new DivStateBlock({
                    height: new MatchParentSize(),
                    width: new MatchParentSize(),
                    div_id: 'screens',
                    states: [
                        {
                            state_id: 'init',
                            div: InitCardTemplate(),
                        },
                        {
                            state_id: 'screen1',
                            div: new ContainerBlock({
                                id: 'ss1',
                                background: [new SolidBackground({
                                    color: '#151517',
                                })],
                                orientation: 'overlap',
                                height: new MatchParentSize(),
                                visibility_action: {
                                    log_id: 'carousel_show',
                                    log_limit: 0,
                                    payload: {
                                        carousel_id: data.GalleryId,
                                        title: data.GalleryTitle,
                                        place: data.GalleryParentScreen,
                                        position: data.GalleryPosition,
                                    },
                                    url: 'metrics://carouselShow',
                                    visibility_duration: 1000,
                                },
                                items: [
                                    new DivStateBlock({
                                        div_id: 'cards',
                                        height: new MatchParentSize(),
                                        states: data.Cards.map((cardData, index) => {
                                            return {
                                                state_id: `card${index}`,
                                                div: GetDocumentTemplate(cardData, data, index),
                                            };
                                        }),
                                    }),
                                    GetIndicatorTemplate(),
                                ],
                            }),
                        },
                    ],
                }),
            },
        ],
    });
};
