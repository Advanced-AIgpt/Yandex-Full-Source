interface Entity {
    type: string;
    value: any;
    tokens: {
        start: number;
        end: number;
    };
}

enum EntityType {
    Geo = 'YANDEX.GEO',
    Name = 'YANDEX.FIO',
    Number = 'YANDEX.NUMBER',
    Datetime = 'YANDEX.DATETIME',
}

interface GeoEntity extends Entity {
    type: EntityType.Geo;
    value: {
        city?: string;
        house_number?: string;
        street?: string;
    };
}

interface NameEntity extends Entity {
    type: EntityType.Name;
    value: {
        first_name: string;
        last_name: string;
    };
}

interface NumberEntity extends Entity {
    type: EntityType.Number;
    value: number;
}

interface DatetimeEntity extends Entity {
    type: EntityType.Datetime;
    value: {
        day: number;
        day_is_relative: boolean;
    };
}

type AnyEntity = GeoEntity | NameEntity | NumberEntity | DatetimeEntity;

export interface Slot {
    type: string;
    tokens: {
        start: number;
        end: number;
    };
    value: string;
}

export type Slots = Record<string, Slot>;

export interface Intent {
    slots: Slots;
}

export interface AudioPlayerState {
    token: string;
    offset_ms: number;
    state: string;
}

export interface UserInfo {
    user_id: string;
    access_token?: string;
    skill_products?: SkillProducts[];
    location?: Location;
    accepted_user_agreements?: boolean;
}

export interface Location {
    lat: number;
    lon: number;
    accuracy: number;
}

export interface SkillProducts {
    uuid: string;
    name: string;
}

export enum AudioPlayerIntentNames {
    Continue = 'YANDEX.PLAYER.CONTINUE',
    Next = 'YANDEX.PLAYER.NEXT',
    Prev = 'YANDEX.PLAYER.PREVIOUS',
}

export enum ImplicitDiscoveryIntentNames {
    TopUpPhone = 'YANDEX.FINANCE.TOP_UP_MOBILE_PHONE'
}

export interface IncomingMessage {
    request: {
        type: string;
        original_utterance: string;
        command: string;
        nlu: {
            tokens: string[];
            entities: AnyEntity[];
            intents?: Record<AudioPlayerIntentNames, Intent>
                & Record<ImplicitDiscoveryIntentNames, Intent>
                & Record<string, Intent>;
        };
        product_uuid?: string,
        product_name?: string,
        error?: string,
        payload?: any;
    };
    account_linking_complete_event?: {};
    session: {
        user_id: string;
        user: UserInfo;
        location?: Location;
    };
    version: string;
    state?: {
        user?: any;
        session?: any;
        audio_player?: AudioPlayerState;
    };
}

export function extractCity(message: IncomingMessage): string | null {
    const {entities} = message.request.nlu;

    for (const entity of entities) {
        if (entity.type === EntityType.Geo && entity.value.city) {
            const {city} = entity.value;

            if (city) {
                return city;
            }
        }
    }

    return null;
}

export interface Response {
    text: string;
    tts?: string;
    card?: {
        type: 'BigImage';
        image_id: string;
        mds_namespace?: string;
        title: string;
        description: string;
        button?: {
            text: string;
            url: string;
            payload?: any;
        };
    };
    buttons?: Array<{
        title: string;
        payload?: any;
        url?: string;
        hide?: boolean;
    }>;
    end_session?: boolean;
}

export interface OutgoingMessage {
    response: Response;
    session: {
        user_id: string;
    };
    version: string;
}
