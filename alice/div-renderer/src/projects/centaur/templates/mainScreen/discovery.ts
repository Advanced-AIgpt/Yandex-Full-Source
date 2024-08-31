import {
    ContainerBlock,
    FixedSize,
    GalleryBlock,
    ImageBackground,
    IndicatorBlock,
    MatchParentSize,
    PagerBlock,
    PageSize,
    PercentageSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import {
    colorWhiteOpacity50,
    colorWhiteOpacity90,
    offsetFromEdgeOfScreen,
    paginationActiveColor,
    paginationNotActiveColor,
} from '../../style/constants';
import { title32m, title44m, title48m } from '../../style/Text/Text';
import { CrutchLoader } from '../../components/Loader/Loader';
import { EnumLayer } from '../../actions/client';
import { getS3Asset } from '../../helpers/assets';
import { textAction } from '../../../../common/actions';

const s3Url = (part: string) => getS3Asset(`discovery/${part}`);

type DiscoveryCardProps = {
    title: string;
    bg: string;
    description: string;
    layer?: EnumLayer;
};
const discoveryCard = ({ title, bg, description, layer = EnumLayer.dialog }: DiscoveryCardProps) =>
    new ContainerBlock({
        background: [
            new ImageBackground({ image_url: bg }),
        ],
        width: new FixedSize({ value: 468 }),
        height: new FixedSize({ value: 275 }),
        actions: [
            CrutchLoader(layer),
            {
                log_id: 'discovery.execute',
                url: textAction(description),
            },
        ],
        border: {
            corner_radius: 28,
        },
        paddings: {
            top: 24,
            left: 24,
            right: 24,
            bottom: 24,
        },
        items: [
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                text: title,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
            }),
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity90,
                text: description,
                width: new MatchParentSize(),
                height: new MatchParentSize(),
                text_alignment_vertical: 'bottom',
            }),
        ],
    });

export const DISCOVERY_CARDS: DiscoveryCardProps[] = [
    { bg: s3Url('news.png'), title: 'Новости', description: 'Расскажи новости технологий' },
    { bg: s3Url('radio.png'), title: 'Послушать любимое', description: 'Включи радио Серебряный дождь', layer: EnumLayer.content },
    { bg: s3Url('usd.png'), title: 'Быть в курсе', description: 'Какой курс доллара?' },
    { bg: s3Url('talk.png'), title: 'Поговорить по душам', description: 'Давай поболтаем' },
    { bg: s3Url('fairy-tale.png'), title: 'Вспомнить детство', description: 'Включи сказки', layer: EnumLayer.content },
    { bg: s3Url('anecdote.png'), title: 'Поднять настроение', description: 'Расскажи анекдот' },
    { bg: s3Url('adventure.png'), title: 'Ждать путешествия', description: 'Как по-итальянски будет бабочка' },
    { bg: s3Url('motivation.png'), title: 'Найти мотивацию', description: 'Кто такой Фил Найт' },
    { bg: s3Url('film.png'), title: 'Провести вечер', description: 'Включи фильм Она', layer: EnumLayer.content },
];

type DiscoveryOptionProps = {
    title: string;
    text: string[];
};
const discoveryOption = ({ title, text }: DiscoveryOptionProps) =>
    new ContainerBlock({
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            new TextBlock({
                ...title44m,
                text_color: colorWhiteOpacity50,
                text: title,
            }),
            new TextBlock({
                ...title48m,
                text_color: colorWhiteOpacity90,
                text: text.map(t => `— ${t}`).join('\n'),
                margins: {
                    top: 16,
                },
            }),
        ],
    });

const DISCOVERY_OPTIONS: DiscoveryOptionProps[] = [
    { title: 'Слушайте музыку', text: ['Включи бодрый рок', 'Поставь мою музыку вперемешку', 'Включи детскую музыку'] },
    { title: 'Узнавайте погоду', text: ['Какая погода на выходных?', 'Будет ли дождь?', 'Тепло ли сейчас в Сочи?'] },
    { title: 'Просыпайтесь вовремя', text: ['Поставь будильник на 8 утра', 'Разбуди меня веселой музыкой', 'Отложи на 5 минут'] },
    { title: 'Засекайте время', text: ['Поставь таймер на 10 минут', 'Сколько до конца таймера?', 'Отмени все таймеры'] },
    { title: 'Узнавайте новое', text: ['Расскажи новости спорта', 'Прочитай новости vc.ru', 'Какие новости в Москве?'] },
];

export const renderDiscoveryTab = ({ Id }: NAlice.NData.ITCentaurMainScreenDiscoveryTabData) => {
    return new TemplateCard(new Templates({}), {
        log_id: Id ?? 'main_screen.tab.discovery',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new WrapContentSize(),
                    items: [
                        new GalleryBlock({
                            width: new WrapContentSize(),
                            height: new WrapContentSize(),
                            item_spacing: 28,
                            paddings: {
                                left: offsetFromEdgeOfScreen,
                                right: offsetFromEdgeOfScreen,
                            },
                            items: DISCOVERY_CARDS.map(discoveryCard),
                        }),
                        new PagerBlock({
                            margins: {
                                top: 52,
                            },
                            id: 'main.pager',
                            height: new WrapContentSize(),
                            width: new MatchParentSize(),
                            layout_mode: new PageSize({ page_width: new PercentageSize({ value: 100 }) }),
                            items: DISCOVERY_OPTIONS.map(discoveryOption),
                        }),
                        new IndicatorBlock({
                            width: new MatchParentSize(),
                            height: new FixedSize({ value: 10 }),
                            margins: {
                                top: 28,
                            },
                            pager_id: 'main.pager',
                            active_item_color: paginationActiveColor,
                            inactive_item_color: paginationNotActiveColor,
                            active_item_size: 1,
                            space_between_centers: new FixedSize({ value: 32 }),
                            shape: {
                                type: 'rounded_rectangle',
                                item_height: new FixedSize({ value: 10 }),
                                item_width: new FixedSize({ value: 20 }),
                                corner_radius: new FixedSize({ value: 20 }),
                            },
                        }),
                    ],
                }),
            },
        ],
    });
};
