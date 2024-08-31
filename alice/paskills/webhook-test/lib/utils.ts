import { parse } from 'url';

export const extractAvatarId = (url: string) => {
    // extracts '<bucket>/<id>' from 'https://<host>/<namespace>/<bucket>/<id>/<size>'
    return parse(url).path!.split('/').slice(2, 4).join('/');
};

export const formatJSON = (str: string, indent: number) => {
    const json = JSON.parse(str);
    const lines = JSON.stringify(json, null, 2).split('\n');
    const prefix = ' '.repeat(indent);

    return lines.map(line => prefix + line).join('\n');
}

export const assertNever = (x: never) => {
    return new Error();
}