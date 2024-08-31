import block from 'bem-cn';
import React, { FC, Fragment, useEffect, useMemo, useState } from 'react';
import { Route, Switch, useParams } from 'react-router-dom';
import { useRoutes } from '../../context/routes';
import { useConsoleApi } from '../../context/console-api';
import moment from '../../lib/moment';
import { IOperation } from '../../model/operation';
import CopyField from '../CopyField/CopyField';
import Link from '../Link/Link';
import Loading from '../Loading/Loading';
import { Button } from 'lego-on-react';
import './History.scss';
import { serializeQueryParams } from '../../lib/utils';

interface ModalProps {
    close: () => void;
    id: string;
}

interface ListProps {
    list: IOperation[];
}

const ListTitle: FC = () => {
    const b = block('HistoryListItem');

    return (
        <div className={b({ title: true })}>
            <div className={b('time')}>Время</div>
            <div className={b('label')}>Событие</div>
            <div className={b('user')}>Инициатор</div>
            <div className={b('kolonkish')}>Временный пользователь</div>
            <div className={b('ctx')}>Контекст</div>
            <div className={b('status')}>Статус</div>
            <div className={b('device')}>Устройство</div>
        </div>
    );
};

const Item: FC<{ operation: IOperation }> = ({ operation }) => {
    const routes = useRoutes();
    const b = block('HistoryListItem');
    const { status, date, type, id, kolonkish, userLogin, deviceId, devicePk } = operation;

    const { organizationId } = useParams();
    const operationMap = {
        activate: 'Активация',
        reset: 'Сброс',
        'promo-activate': 'Активация промокода',
    };

    const renderContext = () => {
        switch (operation.context) {
            case 'customer':
                return <span>Посетитель</span>;
            case 'ext':
                return <span>API</span>;
            case 'int':
                return <span>Админка</span>;
        }

        return null;
    };

    return (
        <div className={b()}>
            <div className={b('time')}>{moment(date).format('H:mm:ss')}</div>
            <div className={b('label', { type })}>{operationMap[type]}</div>
            <div className={b('user')}>{userLogin}</div>
            <div className={b('kolonkish')}>{kolonkish && kolonkish.login}</div>
            <div className={b('ctx')}>{renderContext()}</div>
            <div className={b('status')}>
                <span className={b('status-id', { theme: status })}>
                    <CopyField
                        tooltipTitle='ID операции скопирован'
                        label={operation.status.toUpperCase()}
                        value={id}
                    />
                </span>
            </div>
            {deviceId && organizationId && (
                <div className={b('device')}>
                    <Link to={routes.app.deviceHistory(organizationId, devicePk)}>{deviceId}</Link>
                </div>
            )}
        </div>
    );
};

const HistoryList: FC<ListProps> = ({ list }) => {
    const groupList = useMemo(
        () => {
            return list.reduce(
                (acc, op, i) => {
                    const getCur = () => moment(moment(list[i].date).format('YYYY-MM-DD'));
                    const getPrev = () => moment(moment(list[i - 1].date).format('YYYY-MM-DD'));
                    const diff = i > 0 ? getPrev().diff(getCur(), 'days') : 1;
                    if (diff > 0) {
                        acc.push([]);
                    }
                    acc[acc.length - 1].push(op);
                    return acc;
                },
                [] as IOperation[][],
            );
        },
        [list.length],
    );

    const renderDayTitle = (date: string) =>
        moment(date)
            .calendar()
            .split(' ')
            .map((x, index) => (
                <Fragment key={index}>
                    {x}
                    <br />
                </Fragment>
            ));

    const b = block('HistoryList');
    return (
        <div className={b()}>
            {groupList.map((group) => (
                <div key={group[0].date} className={b('day')}>
                    <div className={b('day-title')}>{renderDayTitle(group[0].date)}</div>
                    <div className={b('day-list')}>
                        {group.map((op) => (
                            <Item key={op.id} operation={op} />
                        ))}
                    </div>
                </div>
            ))}
        </div>
    );
};

export const HistoryPageWrap = () => {
    const routes = useRoutes();

    return (
        <Switch>
            <Route component={HistoryPage} path={routes.app.deviceHistory(':organizationId', ':devicePk')} />
            <Route component={HistoryPage} path={routes.app.history(':organizationId')} />
        </Switch>
    );
};

const HistoryPage: FC = () => {
    const b = block('History');

    const routes = useRoutes();
    const api = useConsoleApi();
    const { devicePk, organizationId } = useParams<{ devicePk?: string; organizationId: string }>();
    const [loading, setLoading] = useState(true);
    const [operations, setOperations] = useState<IOperation[]>([]);
    const [nextTimestamp, setNextTimestamp] = useState(undefined as string | undefined);

    useEffect(
        () => {
            setOperations([]);
            setNextTimestamp(undefined);
            void loadNext();
        },
        [devicePk, organizationId],
    );

    const loadNext = async (nextTS?: string) => {
        setLoading(true);
        try {
            const res = await api.getOrganizationHistory(organizationId, nextTS, devicePk);
            setOperations((prev) => [...prev, ...res.operations]);
            setNextTimestamp(res.next);
        } finally {
            setLoading(false);
        }
    };

    const deviceRoute = routes.app.deviceList(organizationId) + '?' + serializeQueryParams({ q: devicePk });

    return (
        <div className={b('list')}>
            {devicePk && (
                <div className={b('tools')}>
                    <Link to={deviceRoute}>
                        <Button theme='clear' size='s'>
                            К устройству
                        </Button>
                    </Link>
                    <Link to={routes.app.history(organizationId)}>
                        <Button theme='clear' size='s'>
                            Показать все события
                        </Button>
                    </Link>
                </div>
            )}
            <ListTitle />
            <HistoryList list={operations} />
            <div className={b('next-btn')}>
                {loading ? (
                    <Loading size='s' centered />
                ) : (
                    <Button
                        theme='action'
                        size='s'
                        disabled={!Boolean(nextTimestamp)}
                        onClick={() => loadNext(nextTimestamp)}
                    >
                        Показать еще
                    </Button>
                )}
            </div>
        </div>
    );
};
