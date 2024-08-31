import logging
import os
from vins_core.ner.fst_parser import NluFstParser
from vins_core.ner.fst_num import NluFstNum
from vins_core.ner.fst_datetime_ru import NluFstDatetimeRu
from vins_core.ner.fst_datetime_range import NluFstDatetimeRange
from vins_core.ner.fst_geo import NluFstGeo
from vins_core.ner.fst_post import NluFstPost
from vins_core.ner.fst_custom import NluFstCustom, NluFstCustomHierarchy
from vins_core.ner.fst_float import NluFstFloat
from vins_core.ner.fst_calc import NluFstCalc
from vins_core.ner.fst_weekdays import NluFstWeekdays
from vins_core.ner.fst_units import NluFstUnits
from vins_core.ner.fst_time import NluFstTime
from vins_core.utils.data import get_resource_full_path


logger = logging.getLogger(__name__)


PARSER_RU_BASE_PARSERS = (
    'units_time',
    'datetime',
    'date',
    'time',
    'geo',
    'num',
    'fio',
    'datetime_range',
    'poi_category_ru',
    'currency',
    'float',
    'calc',
    'weekdays',
    'soft',
    'site',
    'swear',
)


FST_PARSERS = {
    'units_time': NluFstUnits,
    'datetime': NluFstDatetimeRu,
    'date': NluFstDatetimeRu,
    'time': NluFstTime,
    'geo': NluFstGeo,
    'num': NluFstNum,
    'fio': NluFstCustomHierarchy,
    'datetime_range': NluFstDatetimeRange,
    'poi_category_ru': NluFstCustom,
    'currency': NluFstCustom,
    'float': NluFstFloat,
    'calc': NluFstCalc,
    'weekdays': NluFstWeekdays,
    'soft': NluFstCustom,
    'site': NluFstCustom,
    'album': NluFstCustom,
    'artist': NluFstCustom,
    'track': NluFstCustom,
    'films_100_750': NluFstCustom,
    'films_50_filtered': NluFstCustom,
    'swear': NluFstCustom,
}


class FstParserFactory(object):
    def __init__(self, resource, parser_factories, lang='ru'):
        self._parser_factories = parser_factories
        self._resource = resource
        self._parsers = {}
        self._lang = lang

    def load(self):
        if not self._resource:
            logger.info('Nothing to load: empty resource provided')
            return
        fst_dir = get_resource_full_path(self._resource)
        for name, factory in self._parser_factories.iteritems():
            self._parsers[name] = factory(fst_name=name, fst_dir=os.path.join(fst_dir, 'fst', self._lang))

    def create_parser(self, fst_parsers, additional_parsers=None):
        parsers = []
        additional_parsers = additional_parsers or []
        for name in fst_parsers:
            parser = self._parsers.get(name)
            if not parser:
                raise ValueError('No such parser with name %s' % name)
            parsers.append(parser)
        return NluFstParser(
            normalizer=None,
            parsers=parsers + additional_parsers,
            post=NluFstPost()
        )

    @classmethod
    def from_config(cls, cfg):
        try:
            parser_factories = {name: FST_PARSERS[name] for name in cfg.get('parsers', [])}
        except KeyError as e:
            raise ValueError('Unknown parser with name "%s"' % e.args[0])
        factory = cls(cfg.get('resource'), parser_factories)
        return factory
