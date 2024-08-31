package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;

import javax.annotation.Nullable;

public class UniqueList<T> implements List<T> {

    private static final UniqueList EMPTY = new UniqueList<>(Collections.emptyList());

    protected final List<T> storage;

    private UniqueList(List<T> storage) {
        this.storage = storage;
    }

    public UniqueList() {
        this(new ArrayList<>());
    }

    public UniqueList(int capacity) {
        this(new ArrayList<>(capacity));
    }

    public UniqueList(UniqueList<T> other) {
        this(new ArrayList<>(other));
    }

    public static <T1> UniqueList<T1> from(List<T1> source) {
        return new UniqueList<>(new ArrayList<>(source));
    }

    public static <T1> UniqueList<T1> empty() {
        return (UniqueList<T1>) EMPTY;
    }

    @Override
    public int size() {
        return storage.size();
    }

    @Override
    public boolean isEmpty() {
        return storage.isEmpty();
    }

    @Override
    public boolean contains(Object o) {
        return storage.contains(o);
    }

    @Override
    public Iterator<T> iterator() {
        return storage.iterator();
    }

    @Override
    public Object[] toArray() {
        return storage.toArray();
    }

    @Override
    public <T1> T1[] toArray(T1[] a) {
        return storage.toArray(a);
    }

    @Override
    public boolean add(T t) {
        if (!contains(t)) {
            storage.add(t);
        }
        return true;
    }

    @Override
    public boolean remove(Object o) {
        return storage.remove(o);
    }

    @Override
    public boolean containsAll(Collection<?> c) {
        return storage.containsAll(c);
    }

    @Override
    public boolean addAll(Collection<? extends T> c) {
        for (var element: c) {
            add(element);
        }
        return true;
    }

    @Override
    public boolean addAll(int index, Collection<? extends T> c) {
        for (var element: c) {
            add(element);
        }
        return true;
    }

    @Override
    public boolean removeAll(Collection<?> c) {
        return storage.removeAll(c);
    }

    @Override
    public boolean retainAll(Collection<?> c) {
        return storage.retainAll(c);
    }

    @Override
    public void clear() {
        storage.clear();
    }

    @Override
    public T get(int index) {
        return storage.get(index);
    }

    @Override
    public T set(int index, T element) {
        if (storage.contains(element)) {
            throw new UnsupportedOperationException(String.format("Cannot insert element at position %d because it " +
                    "is already stored in collection", index));
        }
        return storage.set(index, element);
    }

    @Override
    public void add(int index, T element) {
        if (!contains(element)) {
            storage.add(index, element);
        }
    }

    @Override
    public T remove(int index) {
        return storage.remove(index);
    }

    @Override
    public int indexOf(Object o) {
        return storage.indexOf(o);
    }

    @Override
    public int lastIndexOf(Object o) {
        return storage.lastIndexOf(o);
    }

    @Override
    public ListIterator<T> listIterator() {
        return storage.listIterator();
    }

    @Override
    public ListIterator<T> listIterator(int index) {
        return storage.listIterator(index);
    }

    @Override
    public List<T> subList(int fromIndex, int toIndex) {
        return storage.subList(fromIndex, toIndex);
    }

    @Nullable
    public T last() {
        if (isEmpty()) {
            return null;
        }
        return get(size() - 1);
    }

}
