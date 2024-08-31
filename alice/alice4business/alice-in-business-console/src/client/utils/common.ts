export function assignField<T, K extends keyof any, V>(obj: T, field: K, value: V) {
    return Object.assign({}, obj, { [field]: value });
}

export function getKeys<T>(obj: T) {
    return Object.keys(obj) as Array<keyof T>;
}
export function selectTextContent(domNode: HTMLElement) {
    const selection = window.getSelection();

    const range = document.createRange();
    range.selectNodeContents(domNode);

    selection!.removeAllRanges();
    selection!.addRange(range);
}

export function makeSafe<A extends any[], R>(fn: (...args: A) => Promise<R>) {
    return async (...args: A) => {
        try {
            return await fn(...args);
        } catch (e) {
            console.error(e);
        }
    };
}

export const ucfirst = (s: string) => s[0].toUpperCase() + s.slice(1);
