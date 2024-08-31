#!/usr/bin/env python
# encoding: utf-8


class Size():
    def __init__(self):
        self.max = None
        self.min = None
        self.count = 0
        self.sum = 0.0

    @property
    def avg(self):
        if self.count == 0:
            return None
        return self.sum / self.count

    def update(self, val):
        if self.max is None:
            self.max = val
            self.min = val
        else:
            self.max = max(self.max, val)
            self.min = min(self.min, val)

        self.count += 1
        self.sum += val

    def __repr__(self):
        return 'Size(avg={0.avg:.2f}, max={0.max}, min={0.min})'.format(self)


def _meas(subtree, width, depth, cur_depth):
    if isinstance(subtree, dict):
        width.update(len(subtree))
        for sub in subtree.itervalues():
            _meas(sub, width, depth, cur_depth+1)
    else:
        depth.update(cur_depth)


def measure(tree):
    """
    Измерение размеров дерева - ширины на каждом уровне, и глубины листьев
    :param dict tree: Представление дерева в виде словаря со вложенными словарями
    :rtype: tuple[Size, Size]
    :return:
    """
    width = Size()
    depth = Size()
    _meas(tree, width, depth, 1)
    return width, depth

