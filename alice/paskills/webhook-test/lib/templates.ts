import { compile, registerHelper, SafeString } from 'handlebars';

const sample = (xs: any[]) => xs[Math.floor(Math.random() * xs.length)];
const morph = (n: number, titles: string[]) => {
    return titles[(n%10==1 && n%100!=11) ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2];
}

export function prepare<T extends object | undefined = undefined>(x: string, xx: string[][] = []) {
    x = x
        .replace(/^\s*\n/, '')  // trim leading newline
        .replace(/\n\s*$/, ''); // trim trailing newline

    const matches = x.match(/^(\s+)/); // leading line indent

    if (matches) {
        const [indent] = matches;
        const regex = new RegExp('^' + indent, 'gm');

        x = x.replace(regex, '');
    }

    // random helper shorthand
    x = x.replace(/<([^>]+)>/g, '{{random "$1"}}');

    const replacements = xx.reduce((acc, [first, ...rest]) => ({
        ...acc,
        [first]: [first, ...rest],
    }), {});

    const template = compile(x);

    function build(): string;
    function build<P extends T>(params?: P): string;
    function build<P extends T>(params?: P) {
        return template({ replacements, ...params });
    }

    return build;
}

registerHelper('random', (text, params) => {
    const { replacements = [] } = params.data.root;

    const variants = replacements[text];

    if (!variants) {
        throw new Error(`No variants provided for fragment '${text}'`)
    }

    return new SafeString(sample(variants));
});

registerHelper('list', (items) => {
    const lastItem = items.pop();

    return items.join(', ') + ' или ' + lastItem;
});

registerHelper('morph', (n, titles) => {
    return morph(n, titles);
});

