cdef extern from "<alice/nlu/granet/lib/utils/rle_array.h>" namespace "NGranet" nogil:
    cdef cppclass TRleArray[T]:
        cppclass iterator:
            T& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)

        cppclass reverse_iterator:
            T& operator*()
            reverse_iterator operator++()
            reverse_iterator operator--()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)

        cppclass const_iterator(iterator):
            pass

        cppclass const_reverse_iterator(reverse_iterator):
            pass

        TRleArray() except +
        TRleArray(size_t) except +
        TRleArray(size_t, const T&) except +

        iterator begin()
        const_iterator const_begin "begin"()
        bint empty()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        size_t size()
