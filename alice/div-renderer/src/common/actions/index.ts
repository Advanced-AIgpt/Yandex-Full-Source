export type Directive = {
    type: 'client_action' | 'server_action';
    name: string;
    payload?: unknown;
}

export const directivesAction = (directives: Directive[]|Directive) => {
    if (!Array.isArray(directives)) {
        directives = [directives];
    }

    return `dialog://?directives=${encodeURIComponent(JSON.stringify(directives))}`;
};

export const directivesActionWithVariables = (directives: Directive[]|Directive) => {
    if (!Array.isArray(directives)) {
        directives = [directives];
    }

    return `dialog://?directives=@{encodeUri('${JSON.stringify(directives)}')}`;
};

export const textAction = (text: string) => {
    return `dialog://text_command?query=${encodeURI(text)}`;
};
