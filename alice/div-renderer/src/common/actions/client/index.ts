export interface IDirectiveClientAction {
    type: 'client_action',
    name: string,
    payload?: unknown,
}

export const createClientAction = (name: string, payload?: unknown): IDirectiveClientAction => {
    const directive: IDirectiveClientAction = {
        type: 'client_action',
        name,
    };

    if (payload) {
        directive.payload = payload;
    }

    return directive;
};
