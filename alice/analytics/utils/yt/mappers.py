#!/usr/bin/env python
# encoding: utf-8
"""
Мапперы общего назначения
"""
from itertools import product, izip

from nile.api.v1 import Record


class CartesianMultiplier(object):
    """
    Размножает записи путём декартового произведения значений для указанных полей
    Пример использования:
    ```
    from functools import partial
    from nile.api.v1 import clusters, files
    from mappers import CartesianMultiplier

    job = clusters.Hahn().env(files=[
            files.LocalFile('mappers.py'),
        ]).job()

    table = job.table('inp_name').map(CartesianMultiplier({
        'intent': CartesianMultiplier.list_prefixes,
        'form_name': partial(CartesianMultiplier.list_prefixes, separator='.'),
        'app': CartesianMultiplier.with_total,
    }))
    table.put('out_name')

    job.run()
    ```
    """
    def __init__(self, fields):
        """
        :param dict[str, Callable] fields: Поля и функции-мультипликаторы
        Функция-мультипликатор принимает реальное значение указанного поля и возвращает список значений для него.
        Если оригинальное значение тоже нужно - оно должно быть отдано из мультипликатора наравне с остальными.
        Если мультипликатор вернёт пустой список, то запись будет пропущена.
        """
        self.fields = fields

    def __call__(self, records):
        for rec in records:
            dct = rec.to_dict()
            variants = [fun(dct.get(name))
                        for name, fun in self.fields.iteritems()]
            for combination in product(*variants):
                dct.update(izip(self.fields, combination))
                yield Record(**dct)

    # Стандартные мультипликаторы
    # Для кастомной настройки можно завернуть в partial

    @staticmethod
    def with_total(value, total_name='_total_'):
        return [value, total_name]

    @staticmethod
    def list_prefixes(value, separator='\t'):
        """
        CartesianMultiplier.list_prefixes("pa\thandcrafted\tyes_or_no")
        =>  ["pa", "pa\thandcrafted", "pa\thandcrafted\tyes_or_no"]
        """
        if value is None:
            return [value]

        parts = []
        prefixes = []
        for sub in value.split(separator):
            parts.append(sub)
            prefixes.append(separator.join(parts))
        return prefixes

