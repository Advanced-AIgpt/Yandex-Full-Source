import React, { createContext, FC, useContext } from 'react';
import Routes, { IRoutes, RoutesParams } from '../lib/routes';

const RoutesCtx = createContext({} as IRoutes);

const RoutesProvider: FC<RoutesParams> = ({ children, ...params }) => (
    <RoutesCtx.Provider value={new Routes(params)}>{children}</RoutesCtx.Provider>
);

export const useRoutes = () => useContext(RoutesCtx);
export default RoutesProvider;
