export function formatBytes(bytes: number, decimals = 2) {
    if (bytes === 0) {
        return '0 B';
    }

    const k = 1024;
    const dm = decimals < 0 ? 0 : decimals;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'];

    const i = Math.floor(Math.log(bytes) / Math.log(k));

    return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
}

export const searchSubStr = (entry: string, subStr: string) => entry.toLowerCase().indexOf(subStr.toLowerCase()) > -1;

export const truncate = (word: string, len: number) => (word.length > len ? word.slice(0, len) + '...' : word);

export const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const serializeQueryParams = (query: string | Record<string, string | undefined>) => {
    if (typeof query === 'string') {
        return JSON.parse(
            '{"' +
                decodeURI(query)
                    .replace('?', '')
                    .replace(/"/g, '\\"')
                    .replace(/&/g, '","')
                    .replace(/=/g, '":"') +
                '"}',
        );
    }
    return Object.keys(query)
        .map((key) =>
            typeof query[key] === 'string' ? encodeURIComponent(key) + '=' + encodeURIComponent(query[key]!) : null,
        )
        .filter(Boolean)
        .join('&');
};

export const rtrim = (char: string, url: string) => {
    if (char === '' || !url) {
        return url;
    }

    while (url.endsWith(char)) {
        url = url.slice(0, -1);
    }

    return url;
};
