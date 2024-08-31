import React, { createContext, FC, useContext } from 'react';
import ConsoleAPI, { IConsoleApi } from '../lib/console-api';
import { ApiParams } from '../lib/api';

const ConsoleApiContext = createContext({} as IConsoleApi);

const ConsoleApiProvider: FC<ApiParams> = ({ children, ...params }) => (
    <ConsoleApiContext.Provider value={new ConsoleAPI(params)}>{children}</ConsoleApiContext.Provider>
);

export const useConsoleApi = () => useContext(ConsoleApiContext);
export default ConsoleApiProvider;
