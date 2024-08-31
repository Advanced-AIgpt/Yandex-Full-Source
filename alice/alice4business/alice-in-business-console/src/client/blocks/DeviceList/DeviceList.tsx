import block from 'bem-cn';
import { observer } from 'mobx-react';
import React, { useState, useEffect } from 'react';
import { IDevice } from '../../model/device';
import { DeviceListStore } from '../../store/device-list';
import Device from '../Device/Device';
import Row from '../Row/Row';
import './DeviceList.scss';
import FilterTools from './FilterTools';
import { OrganizationStore } from '../../store/organization';
import { RoomListStore } from '../../store/room-list';
import { DeviceStore } from '../../store/device';
import { PaginationList, PaginationState } from '../PaginationList/PaginationList';

const b = block('DeviceList');
const NUMBER_PER_PAGE = 25;

interface Props {
    organization: OrganizationStore;
    devices: DeviceListStore;
    rooms: RoomListStore;
}

const DeviceList = observer(({ devices: deviceList, organization, rooms }: Props) => {
    const [pagination, setPagination] = useState({
        data: deviceList.visibleList as DeviceStore[],
        offset: 0,
        numberPerPage: NUMBER_PER_PAGE,
        pageCount: 0,
        currentData: [] as DeviceStore[]
    } as PaginationState);


    useEffect(() => {
        setPagination((prevState) => ({
            ...prevState,
            data: deviceList.visibleList as DeviceStore[],
            pageCount: deviceList.visibleList.length / NUMBER_PER_PAGE,
            currentData: deviceList.visibleList.slice(prevState.offset, prevState.offset + NUMBER_PER_PAGE)
        }))
    }, [deviceList.visibleList])

    useEffect(() => {
        setPagination({
            data: deviceList.visibleList as DeviceStore[],
            offset: 0,
            numberPerPage: NUMBER_PER_PAGE,
            pageCount: deviceList.visibleList.length / NUMBER_PER_PAGE,
            currentData: deviceList.visibleList.slice(0, NUMBER_PER_PAGE)
        })
    }, [deviceList.searchFilter])

    const onOrderChange = (attr: keyof IDevice) => {
        deviceList.orderAttr = attr;
        deviceList.orderType = deviceList.orderType === 'ascending' ? 'descending' : 'ascending';
    };

    const withOrderControl = ({ title, field: attr }: { title: string; field: keyof IDevice }) => {
        const { orderAttr, orderType } = deviceList;
        const position = attr === orderAttr && orderType === 'descending' ? 'up' : 'down';
        const active = attr === orderAttr;
        return (
            <div className={b('order-title-wrap')}>
                <div className={b('order-title')}>{title}</div>
                <span
                    onClick={() => onOrderChange(attr)}
                    className={b('order-arrow', { position, active })}
                />
            </div>
        );
    };

    return (
        <>
            <FilterTools devicesStore={deviceList} />
            <div className={b('title-row')}>
                <Row
                    deviceId={withOrderControl({ title: 'ID устройства', field: 'deviceId' })}
                    externalDeviceId={withOrderControl({
                        title: 'Идентификатор',
                        field: 'externalDeviceId',
                    })}
                    note={withOrderControl({ title: 'Пометки', field: 'note' })}
                    room={withOrderControl({ title: 'Комната', field: 'room' })}
                    status={withOrderControl({ title: 'Статус', field: 'status' })}
                    controls='Управление'
                    showRoom={organization.usesRooms}
                />
            </div>
            <div className={b()}>
                {pagination.currentData && (
                    <>
                        {pagination.currentData.map((device, i) => (
                                    <Device
                                        key={device.id}
                                        deviceStore={device as DeviceStore}
                                        deviceListStore={deviceList}
                                        organizationStore={organization}
                                        even={Boolean(i % 2)}
                                        roomListStore={rooms}
                                    />
                            ))
                        }
                        <PaginationList pagination={pagination} setPagination={setPagination}/>
                    </>
                )}
            </div>
        </>
    );
});

export default DeviceList;
