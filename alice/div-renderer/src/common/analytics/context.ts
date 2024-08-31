import * as uuid from 'uuid';

export enum MediaType {
    None = 'none',
    Music = 'music',
    Video = 'video',
    // etc
}

export class MediaMeta {
    public readonly mediaType: MediaType;
    public readonly pictureUrl?: (string | null);
    public readonly objectId?: (string | null);
    public readonly objectType?: (string | null);
    public readonly objectUrl?: (string | null);

    public constructor(
        mediaType: MediaType,
        pictureUrl?: (string | null),
        objectId?: (string | null),
        objectType?: (string | null),
        objectUrl?: (string | null)) {
        this.mediaType = mediaType;
        this.pictureUrl = pictureUrl;
        this.objectId = objectId;
        this.objectType = objectType;
        this.objectUrl = objectUrl;
    }
}

export enum ElementType {
    Button = 'button',
    // etc
}

export class Element {
    public readonly elementType: ElementType;
    public readonly elementId: string;
    public readonly title?: (string | null);
    public readonly description?: (string | null);

    public constructor(
        elementType: ElementType,
        title?: (string | null),
        description?: (string | null),
    ) {
        this.elementType = elementType;
        this.elementId = uuid.v4();
        this.title = title;
        this.description = description;
    }
}

// Copy-on-write AnalyticsContext for cascade fullfillment
export class AnalyticsContext {
    public readonly reqId?: (string | null);
    public readonly screenInstanceId?: (string | null);
    public readonly screenTitle?: (string | null);
    public readonly mediaMeta?: (MediaMeta | null);
    public readonly element?: (Element | null);

    private constructor(
        reqId?: (string | null),
        screenInstanceId?: (string | null),
        screenTitle?: (string | null),
        mediaMeta?: (MediaMeta | null),
        element?: (Element | null),
    ) {
        this.reqId = reqId;
        this.screenInstanceId = screenInstanceId;
        this.screenTitle = screenTitle;
        this.mediaMeta = mediaMeta;
        this.element = element;
    }

    public static startWithReqId(reqId?: (string | null)): AnalyticsContext {
        return new AnalyticsContext(reqId);
    }

    public addScreen(screenTitle: string): AnalyticsContext {
        return new AnalyticsContext(this.reqId, uuid.v4(), screenTitle, this.mediaMeta, this.element);
    }

    public addMediaMeta(mediaMeta: MediaMeta): AnalyticsContext {
        return new AnalyticsContext(this.reqId, this.screenInstanceId, this.screenTitle, mediaMeta, this.element);
    }

    public setElement(element: Element): AnalyticsContext {
        return new AnalyticsContext(this.reqId, this.screenInstanceId, this.screenTitle, this.mediaMeta, element);
    }
}
