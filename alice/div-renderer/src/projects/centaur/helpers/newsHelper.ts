import { getStaticS3Asset } from './assets';

const TOPICS = new Set([
    'auto',
    'business',
    'computers',
    'culture',
    'incident',
    'index',
    'koronavirus',
    'personal',
    'politics',
    'science',
    'society',
    'sport',
    'world',
]);

const ALIASES = {
    nplus1: 'science',
} as { [name: string]: string };

export function getImageByTopic(topic: string | null | undefined): string {
    let resultImage = 'news/index.png';

    if (topic) {
        if (Object.prototype.hasOwnProperty.call(ALIASES, topic)) {
            topic = ALIASES[topic];
        }

        if (TOPICS.has(topic)) {
            resultImage = `news/${topic}.png`;
        }
    }
    return getStaticS3Asset(resultImage);
}
