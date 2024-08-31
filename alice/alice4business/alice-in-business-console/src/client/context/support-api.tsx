import React, { createContext, FC, useContext } from 'react';
import SupportAPI , { ISupportApi } from '../lib/support-api';
import { ApiParams } from '../lib/api';

const SupportApiContext = createContext({} as ISupportApi);

const SupportApiProvider: FC<ApiParams> = ({ children, ...params }) => (
    <SupportApiContext.Provider value={new SupportAPI(params)}>{children}</SupportApiContext.Provider>
);

export const useSupportApi = () => useContext(SupportApiContext);
export default SupportApiProvider;
