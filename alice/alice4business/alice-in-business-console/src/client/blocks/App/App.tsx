import block from 'bem-cn';
import React, { createContext, FC, ReactNode, useContext, useState } from 'react';
import { BrowserRouter, Link, Redirect, Route, Switch } from 'react-router-dom';
import RoutesProvider, { useRoutes } from '../../context/routes';
import OrganizationChoicePage from './OrganizationChoicePage';
import OrganizationPage from './OrganizationPage';
import Footer from '../Footer/Footer';
import './App.scss';
import { ModalWrap } from '../../utils/modals/service';
import { AlertWrap } from '../../utils/alerts/service';
import { Header, Image, User, UserAccount } from 'lego-on-react';
import { Config } from '../../../configs/defaults';
import ConsoleApiProvider from '../../context/console-api';

const b = block('App');

const App: FC = () => {
    const routes = useRoutes();
    const yaIconUrl = `https://yastatic.net/q/logoaas/v1/${encodeURIComponent('Яндекс')}.svg?size=27&color=000`;
    const dialogsIconUrl = `https://yastatic.net/q/logoaas/v1/${encodeURIComponent(' Диалоги')}.svg?size=27&color=000`;
    const {
        config: { avatarHost, passportHost },
        user,
        yu,
        users,
    } = useAppStateContext();

    return (
        <div className={b()}>
            <Header
                cls={b('header')}
                logo={
                    <Header.Logo size='m'>
                        <a href='https://yandex.ru'>
                            <Image url={yaIconUrl} />
                        </a>
                        <Link to={routes.app.root()}>
                            <Image url={dialogsIconUrl} />
                        </Link>
                    </Header.Logo>
                }
                left={<></>}
                right={
                    <User
                        passportHost={passportHost}
                        uid={user.uid}
                        yu={yu}
                        name={user.displayName}
                        subname={user.login !== user.displayName ? user.login : undefined}
                        pic
                        avatarId={user.avatar!.default}
                        avatarHost={avatarHost}
                    >
                        {(users || []).map((x) => (
                            <UserAccount
                                key={x.uid}
                                uid={x.uid}
                                name={x.displayName}
                                pic
                                avatarId={x.avatar!.default}
                                avatarHost={avatarHost}
                            />
                        ))}
                    </User>
                }
            />
            <Switch>
                <Route exact path={routes.app.root()} component={OrganizationChoicePage} />
                <Route path={routes.app.organization(':organizationId')} component={OrganizationPage} />
                <Redirect to={routes.app.root()} />
            </Switch>
            <Footer />
        </div>
    );
};

export interface AppProps {
    user: Partial<ExpressBlackbox.User>;
    users?: Array<Partial<ExpressBlackbox.User>>;
    yu: string;
    secretkey: string;
    config: ReturnType<Config['client']>;
}

export interface IAppCtx extends AppProps {
    headerControls: ReactNode;
    setHeaderControls: (component: ReactNode) => void;
}

const AppStateCtx = createContext<IAppCtx>({} as any);

export const useAppStateContext = () => useContext(AppStateCtx);

export default (props: AppProps) => {
    const [headerControls, setHeaderControls] = useState(null as ReactNode);

    return (
        <RoutesProvider
            appUrl={props.config.urlRoot}
            assetsUrl={props.config.assetsRoot}
            apiUrl={props.config.apiRoot}
            connectUrl={props.config.connectHost}
            secretkey={props.secretkey}
        >
            <ConsoleApiProvider
                pollingInterval={props.config.pollTimeout}
                rootUrl={props.config.apiRoot}
                secretkey={props.secretkey}
            >
                <AppStateCtx.Provider value={{ ...props, headerControls, setHeaderControls }}>
                    <AlertWrap>
                        <ModalWrap>
                            <BrowserRouter>
                                <App />
                            </BrowserRouter>
                        </ModalWrap>
                    </AlertWrap>
                </AppStateCtx.Provider>
            </ConsoleApiProvider>
        </RoutesProvider>
    );
};
