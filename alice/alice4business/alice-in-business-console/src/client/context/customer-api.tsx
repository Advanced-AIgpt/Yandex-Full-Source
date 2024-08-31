import React, { createContext, FC, useContext } from 'react';
import { ApiParams } from '../lib/api';
import CustomerApi, { ICustomerApi } from '../lib/customer-api';

const CustomerApiContext = createContext({} as ICustomerApi);

const CustomerApiProvider: FC<ApiParams> = ({ children, ...params }) => (
    <CustomerApiContext.Provider value={new CustomerApi(params)}>{children}</CustomerApiContext.Provider>
);

export const useCustomerApi = () => useContext(CustomerApiContext);
export default CustomerApiProvider;
