import * as React from 'react';

export function createContext<TContextData>(defaultValue: TContextData) {
    const Context = React.createContext<TContextData>(defaultValue);
    return {
        Provider: Context.Provider,
        withContext: createContextConsumerFactory(Context),
        Consumer: Context.Consumer,
        Context,
    };
}

export function createContextConsumerFactory<TContextData>(Context: React.Context<TContextData>) {
    return function withContext<P extends TContextData>(Component: React.ComponentType<P>) {
        return function BoundComponent(props: Pick<P, Exclude<keyof P, keyof TContextData>>) {
            return (
                <Context.Consumer>
                    {(ctx) => (
                        // https://github.com/Microsoft/TypeScript/issues/28748
                        // return <Component { ...props} { ...ctx } />;
                        /// @ts-ignore
                        <Component {...{ ...props, ...ctx } as P} />
                    )}
                </Context.Consumer>
            );
        };
    };
}
