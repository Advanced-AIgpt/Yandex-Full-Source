export interface ColorSet {
    light: string;
    dark: string;
}

export interface ColorScheme {
    [k: string]: ColorSet | ColorScheme;
}

export type ColorNamespace<T extends ColorScheme> = {
    [K in keyof T]: T[K] extends ColorScheme ? ColorNamespace<T[K]> : string;
};

/** https://a.yandex-team.ru/arcadia/portal/morda-schema/api/search/2/paletted-color.json */
export interface PaletteColor {
    name: string;
    color: string;
}

export interface Palette {
    light: PaletteColor[];
    dark: PaletteColor[];
}

function isColorSet(x: ColorSet | ColorScheme): x is ColorSet {
    return Object.keys(x).length === 2 && 'light' in x && 'dark' in x;
}

function colorSchemeToPalette({
    colorScheme,
    scope,
}: {
    colorScheme: ColorScheme;
    scope: string;
}): Palette {
    const palette: Palette = {
        dark: [],
        light: [],
    };

    const stack: {
        path: string[];
        node: ColorScheme | ColorSet;
    }[] = Object.entries(colorScheme).map(([key, value]) => ({
        path: [key],
        node: value,
    }));

    while (stack.length) {
        const { path, node } = stack.pop() as {
            path: string[];
            node: ColorScheme | ColorSet;
        };

        if (isColorSet(node)) {
            const paletteName = `${scope}.${path.join('.')}`;
            palette.light.push({ name: paletteName, color: node.light });
            palette.dark.push({ name: paletteName, color: node.dark });
        } else {
            stack.push(
                ...Object.entries(node).map(([key, value]) => ({
                    path: [...path, key],
                    node: value,
                })),
            );
        }
    }

    return palette;
}

function colorSchemeToNamespace<T extends ColorScheme>({
    colorScheme,
    scope,
}: {
    colorScheme: T;
    scope: string;
}): ColorNamespace<T & typeof commonColorScheme> {
    const rootNamespace: ColorNamespace<ColorScheme> = {};

    const stack: {
        path: string[];
        key: string;
        node: ColorScheme | ColorSet;
        parentNamespace: ColorNamespace<any>;
    }[] = Object.entries(colorScheme).map(([key, value]) => ({
        key,
        path: [key],
        node: value,
        parentNamespace: rootNamespace,
    }));

    while (stack.length) {
        const { path, key, node, parentNamespace } = stack.pop() as {
            path: string[];
            key: string;
            node: ColorScheme | ColorSet;
            parentNamespace: ColorNamespace<any>;
        };

        if (isColorSet(node)) {
            parentNamespace[key] = `@{${scope}.${path.join('.')}}`;
        } else {
            const newNamespace: ColorNamespace<any> = {};
            parentNamespace[key] = newNamespace;

            stack.push(
                ...Object.entries(node).map(([key, value]) => ({
                    key,
                    path: [...path, key],
                    node: value,
                    parentNamespace: newNamespace,
                })),
            );
        }
    }

    return rootNamespace as ColorNamespace<T & typeof commonColorScheme>;
}

export function getPalette<T extends ColorScheme, U extends boolean | undefined = undefined>({
    colorScheme,
    scope,
    extendsCommon,
}: {
    colorScheme: T;
    scope: string;
    extendsCommon?: U;
}): {
    palette: Palette;
    ns: ColorNamespace<U extends true ? T & typeof commonColorScheme : T>;
} {
    if (extendsCommon) {
        colorScheme = {
            ...commonColorScheme,
            ...colorScheme,
        };
    }

    return {
        palette: colorSchemeToPalette({ colorScheme, scope }),
        ns: colorSchemeToNamespace({ colorScheme, scope }) as ColorNamespace<U extends true ? T & typeof commonColorScheme : T>,
    };
}

const commonColorScheme = {
    text: {
        primary: {
            light: '#000000',
            dark: '#ffffff',
        },
    },
};
