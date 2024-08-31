import block from 'bem-cn';
import { RadioButton, TextInput } from 'lego-on-react';
import { observer } from 'mobx-react';
import React from 'react';

import './FilterTools.scss';
import { DeviceListStore } from '../../store/device-list';
import { Status, statusLabels } from '../../model/device';
import { serializeQueryParams } from '../../lib/utils';
import { useHistory, useLocation } from 'react-router-dom';

const b = block('FilterTools');
const size = 's';

interface Props {
    devicesStore: DeviceListStore;
}

const FilterTools = observer(({ devicesStore }: Props) => {
    const history = useHistory();
    const location = useLocation();
    const onSearchInput = (filter: string) => {
        devicesStore.searchFilter = filter;

        const query = location.search ? serializeQueryParams(location.search) : {};
        const cur = { q: filter || undefined };
        history.replace({ search: serializeQueryParams({ ...query, ...cur }) });
    };

    const setCategory = (category?: typeof devicesStore.categoryFilter) => {
        devicesStore.categoryFilter = category || null;
        const query = location.search ? serializeQueryParams(location.search) : {};
        const cur = { category };
        history.replace({ search: serializeQueryParams({ ...query, ...cur }) });
    };

    return (
        <div className={b()}>
            <TextInput
                cls={b('search')}
                theme='normal'
                size={size}
                hasClear
                placeholder='Поиск по устройствам'
                text={devicesStore.searchFilter}
                onChange={onSearchInput}
            />
            <RadioButton
                cls={b('category')}
                theme='normal'
                size={size}
                value={devicesStore.categoryFilter || ''}
                onChange={(e) => setCategory((e.target.value || null) as typeof devicesStore.categoryFilter)}
            >
                <RadioButton.Radio value={Status.Active}>{statusLabels[Status.Active]}</RadioButton.Radio>
                <RadioButton.Radio value={Status.Inactive}>{statusLabels[Status.Inactive]}</RadioButton.Radio>
                <RadioButton.Radio value='offline'>Вне сети</RadioButton.Radio>
                <RadioButton.Radio value=''>Все</RadioButton.Radio>
            </RadioButton>
        </div>
    );
});

export default FilterTools;
