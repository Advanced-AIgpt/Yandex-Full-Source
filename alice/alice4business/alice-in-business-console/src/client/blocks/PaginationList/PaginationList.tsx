import React, { Dispatch, SetStateAction, useCallback, useEffect } from 'react'
import block from 'bem-cn';
import ReactPaginate from 'react-paginate';
import { Button, Icon } from 'lego-on-react';
import { DeviceStore } from '../../store/device';
import { RoomStore } from '../../store/room';
import './PaginationList.scss'

const b = block('Pagination');

export interface PaginationState {
    data: Array<DeviceStore | RoomStore>;
    offset: number;
    numberPerPage: number;
    pageCount: number;
    currentData: Array<DeviceStore | RoomStore>;
}

interface PaginationProps {
    pagination: PaginationState;
    setPagination: Dispatch<SetStateAction<PaginationState>>;
}

export const PaginationList: React.FC<PaginationProps> = ({pagination, setPagination}) => {
    useEffect(() => {
        setPagination((prevState) => ({
            ...prevState,
            data: pagination.data,
            pageCount: pagination.data.length / pagination.numberPerPage,
            currentData: pagination.data.slice(pagination.offset, pagination.offset + pagination.numberPerPage)
        }))
    }, [pagination.numberPerPage, pagination.offset])


    const handlePageClick = useCallback((event) => {
        const selected = event.selected;
        const offset = selected * pagination.numberPerPage
        setPagination({ ...pagination, offset })
    }, [pagination])

    return (
        <>
        {pagination.currentData && (
                <>
                    <ReactPaginate
                        previousLabel={
                            <Button
                                iconLeft={<Icon glyph="type-arrow" direction="left" />}
                                theme="action"
                                size="s"
                                disabled={pagination.offset === 0}
                            >
                                Назад
                            </Button>
                        }
                        nextLabel={
                            <Button
                                iconRight={<Icon glyph="type-arrow" direction="right" />}
                                theme="action"
                                size="s"
                                disabled={pagination.currentData ? pagination.offset >= pagination.data.length - pagination.numberPerPage : false}
                            >
                                Вперёд
                            </Button>
                        }
                        breakLabel={'...'}
                        pageCount={pagination.pageCount}
                        marginPagesDisplayed={2}
                        pageRangeDisplayed={5}
                        onPageChange={handlePageClick}
                        containerClassName={b()}
                        activeClassName={b('Active')}
                    />
                </>
            )}
        </>
    )
}
