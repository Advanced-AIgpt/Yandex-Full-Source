# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_core.common.sample import Sample
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.utils.strings import smart_unicode
from vins_core.ext.geobase import get_geobase

from vins_tools.nlu.ner.fst_utils import put_cases, fst_cases, put_case, fstring, inflect_cases
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor


_normalizer = None

_REGION_TYPE_ID = {
    "continent": 1,
    "country": 3,
    "city": 6,
    "metro_station": 9
}


def normalize(string):
    if not string:
        return string
    global _normalizer
    if not _normalizer:
        from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
        _normalizer = NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)
    sample = Sample.from_string(smart_unicode(string))
    normalized_sample = _normalizer(sample)
    normalized_string = ' '.join(normalized_sample.tokens)
    if '- ' in normalized_string:
        # normalization failed
        normalized_string = ''
    return normalized_string


class NluFstGeoRuConstructor(NluFstBaseValueConstructor):
    def create(self):

        super(NluFstGeoRuConstructor, self).create()
        geo = get_geobase()

        continent, id_to_continent = self._regions("continent", geo)
        country, id_to_country = self._regions("country", geo)
        city, id_to_city = self._regions("city", geo)
        metro_station, id_to_metro_station = self._regions("metro_station", geo, skip_unimportant=False)

        street = self._street()
        street_spec = self._street_form()
        house = self.w + self._house()

        prep = self.w + gn.Fst.union_seq([
            'на', 'в', 'около', 'у', 'по', 'за', 'до'
        ])

        self.maps.update(id_to_continent)
        self.maps.update(id_to_country)
        self.maps.update(id_to_city)
        self.maps.update(id_to_metro_station)

        city_may_be_prep = city + self.w + gn.qq(prep + self.w)

        self.fsts = [
            self.ftag + city,
            self.ftag + country,
            self.ftag + continent,
            self.ftag + metro_station,
            # [Москва] улица Тверская 6
            self.ftag + gn.qq(city_may_be_prep + self.w) + street_spec + self.w + street + house,
            # Москва Тверская улица 6
            self.ftag + city_may_be_prep + self.w + street + self.w + street_spec + house
        ]

    @classmethod
    def _put_cases_geo(cls, region_id, region_name, geobase):
        ln = geobase.linguistics(region_id, b'ru')

        geobase_inflection = dict(zip(
            inflect_cases,
            map(normalize, (ln.nominative, ln.genitive, ln.dative, ln.accusative, ln.instrumental, ln.prepositional))
        ))
        for case in geobase_inflection:
            if not geobase_inflection[case]:
                geobase_inflection[case] = put_case(region_name, case)
        return geobase_inflection

    @classmethod
    def _dump_regions(cls, region_type_id, geobase, skip_unimportant=True):

        id_to_name = {}
        regions = []
        for region in geobase.regions_by_type(region_type_id):
            if region.pos == 0 and skip_unimportant:
                # skip unimportant regions
                continue
            region_name = normalize(region.name)
            if not region_name:
                # skip when normalization fails
                continue
            id_to_name[region.id] = region.name
            regions.append((region_name, region.id))

            for case, region_inflected in cls._put_cases_geo(region.id, region_name, geobase).iteritems():
                if not region_inflected:
                    print 'Missed inflected form in "%s" for region "%s"' % (case, region_name)
                    continue
                regions.append((region_inflected, region.id))

            for region_synonym in region.synonyms.split(b','):
                if not region_synonym:
                    continue
                region_synonym_normalized = normalize(region_synonym)
                if not region_synonym_normalized:
                    continue
                regions.append((region_synonym_normalized, region.id))
                for case, region_synonym_inflected in enumerate(put_cases(region_synonym_normalized)):
                    if not region_synonym_inflected:
                        print 'Missed inflected form in "%s" for region "%s"' % (
                            inflect_cases[case], region_synonym_normalized
                        )
                        continue
                    regions.append((region_synonym_inflected, region.id))
        return regions, id_to_name

    def _regions(self, region_type, geobase, **kwargs):
        regions, id_to_region = self._dump_regions(_REGION_TYPE_ID[region_type], geobase, **kwargs)
        fst_regions = []
        for region_word, region_id in regions:
            fst_regions.append(fstring(region_word) + self._insert(unicode(region_id)))
        print 'Loading %d regions with region type = %s' % (len(fst_regions), region_type)
        fst_regions = gn.Fst.union_seq(fst_regions).optimize()
        return self._insert(region_type + '=') + fst_regions, id_to_region

    def _street_form(self):

        full_names = [
            'улица', 'переулок', 'шоссе', 'проспект', 'проезд',
            'авеню', 'площадь', 'сквер', 'бульвар', 'набережная'
        ]
        short_names = [
            'ул', 'пер', 'ш', 'пр-кт', 'б-р', 'наб', 'пл'
        ]
        return gn.Fst.union_seq(
            sum(map(put_cases, full_names), []) +
            short_names
        )

    def _street(self):
        ordinals = gn.rr(gn.g.digit, 1, 4) + gn.qq('-') + gn.rr(gn.g.russian_letter, 1, 3)
        cardinals = gn.rr(gn.g.digit, 1, 4)
        one_word = (
            gn.g.russian_letter + gn.pp(gn.g.russian_letter)
        )
        possible_street_names = (
            (one_word | cardinals | ordinals) + gn.qq(
                # additional cost used to prevent paths going through other consecutive fsts
                gn.cost(self.w + one_word, 0.1) + gn.qq(
                    gn.cost(self.w + one_word, 0.1)
                )
            )
        )
        return self._insert('street=') + self._catch(possible_street_names)

    def _house_form(self):
        return gn.Fst.union_seq([
            fst_cases('дом'), 'д', fst_cases('строение'), 'стр',
            fst_cases('корпус'), 'кор', 'корп'
        ])

    def _building_form(self):
        return gn.Fst.union_seq(map(fst_cases, [
            'корпус', 'строение', 'подъезд', 'дробь',
            'корп', 'кор', 'к', 'стр', 'лит', '/'
        ]))

    def _house(self):

        f_house_form = self._house_form()
        f_building_form = self._building_form()
        f_house_num = gn.rr(gn.g.digit, 1, 3) + gn.qq(gn.qq(self.w) + gn.anyof(u'абвгдезжклмн'))
        f_house_aux = gn.ss(
            gn.qq(self.w) + f_building_form + gn.qq(self.w) + gn.pp(gn.g.digit)
        )
        return self._insert('house=') + (
            gn.qq(f_house_form + self.w) +
            self._catch(f_house_num + f_house_aux)
        )

    def _disable(self, *fsts):
        return gn.cost(gn.Fst.union_seq(fsts), 0.1)
