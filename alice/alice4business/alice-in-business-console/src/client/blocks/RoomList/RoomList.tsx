import block from 'bem-cn';
import { observer } from 'mobx-react';
import React, { useEffect, useState } from 'react';
import RoomRow from '../RoomRow/RoomRow';
import Room from '../Room/Room';
import { OrganizationStore } from '../../store/organization';
import { RoomListStore } from '../../store/room-list';
import './RoomList.scss';
import { PaginationList, PaginationState } from '../PaginationList/PaginationList';
import { RoomStore } from '../../store/room';

const b = block('RoomList');
const NUMBER_PER_PAGE = 25;

interface Props {
    organization: OrganizationStore;
    rooms: RoomListStore;
}

const RoomList = observer(({ rooms: roomList, organization }: Props) => {
    const [pagination, setPagination] = useState({
        data: roomList.visibleList as RoomStore[],
        offset: 0,
        numberPerPage: NUMBER_PER_PAGE,
        pageCount: 0,
        currentData: [] as RoomStore[]
    } as PaginationState);

    useEffect(() => {
        setPagination((prevState) => ({
            ...prevState,
            data: roomList.visibleList as RoomStore[],
            pageCount: roomList.visibleList.length / NUMBER_PER_PAGE,
            currentData: roomList.visibleList.slice(prevState.offset, prevState.offset + NUMBER_PER_PAGE)
        }))
    }, [roomList.visibleList])

    return (
        <>
            <div className={b('title-row')}>
                <RoomRow
                    name='Комната'
                    externalRoomId='Идентификатор'
                    status='Статус'
                    numDevices='Устройств'
                    controls='Управление'
                />
            </div>
            <div className={b()}>
                {pagination.currentData && (
                    <>
                        {pagination.currentData.map((room, i) => (
                            <Room
                                key={room.id}
                                roomStore={room as RoomStore}
                                roomListStore={roomList}
                                organizationStore={organization}
                                even={Boolean(i % 2)}
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

export default RoomList;
