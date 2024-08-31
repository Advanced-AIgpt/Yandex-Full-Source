#!/usr/bin/env python
# encoding: utf-8
"""
Манипуляции с деревом интентов
"""
from collections import OrderedDict

from utils.json_utils import json_dumps
from exc import DuplicatedSubTree
from tree_size import measure


class LexicDict(dict):
    # Сортировка ключей по алфавиту, с утоплением "other" интентов в конец
    def iteritems(self):
        return sorted(dict.iteritems(self),
                      key=lambda (k, v): ((k.startswith(u'Другое') or
                                           k.startswith(u'Прочее') or
                                           k.startswith(u'Прочие')),
                                          ('other' in v),
                                          k)
                      )


class LayoutIntentTree(object):
    def __init__(self, layout_conf):
        self.layout_conf = layout_conf
        self.chapters = layout_conf.get_yaml_tree()

    @staticmethod
    def get_short_name(key):
        # Вытаскивает из yaml ключа имя, которое будет написано на кнопке
        return key.split('|')[0].strip()

    # Валидация интентов

    _intent2hint = None  # Маркер отсутствия готового кэша

    def map_intents_to_path(self):
        get_short_name = self.get_short_name

        def _rev_path(node, prefix):
            for key, val in node.iteritems():
                path = prefix + [get_short_name(key)]
                if isinstance(val, basestring):
                    self._intent2hint.setdefault(val, path)
                else:
                    _rev_path(val, path)

        if self._intent2hint is None:
            self._intent2hint = {}
            _rev_path(self.chapters, prefix=[])

        return self._intent2hint

    def get_size(self):
        """
        Размер дерева
        :rtype: tuple(Size, Size)
        :return:
          0) Ширина - варианты на одном уровне
          1) Глубина - количество кликов до интента
          Пример: (Size(avg=4.02, max=11, min=2), Size(avg=4.45, max=5, min=3))
        """
        return measure(self.chapters)


class MergedIntentTree(LayoutIntentTree):
    def __init__(self, prj_conf):
        self.prj_conf = prj_conf
        self.layouts_dict = prj_conf.dict_used_layouts()
        self.chapters = self.merge_layouts()

    def merge_layouts(self):
        is_layout = self.prj_conf.is_layout
        layout_key = self.prj_conf.layout_key
        get_short_name = self.get_short_name

        def _find_similar(place, key):
            short = get_short_name(key)
            for ex_key, ex_val in place.iteritems():
                if key == ex_key:
                    return ex_val  # Полностью совпадающие будут слиты
                if short == ex_key.split('|')[0].strip():
                    # В интерфейсе будут одинковые названия, отличаться только подсказками
                    # Если это действительно разные подразделы, нужно назвать совсем по разному
                    # Если один, должны быть полностью одинаковыми в том числе и подзказки
                    msg = u'Разделы с одинаковым названием, но разной подсказкой: "{}", "{}"'.format(key, ex_key)
                    raise DuplicatedSubTree(msg.encode('utf-8'))

        def _subchapter(place, key):
            sub = _find_similar(place, key)
            if sub is None:
                sub = LexicDict()
                place[key] = sub
            elif isinstance(sub, basestring):
                msg = u'Подразедел, или конечный узел? "%s"' % key
                raise DuplicatedSubTree(msg.encode('utf-8'))
            return sub

        def _merge(place, subtree):
            for key, val in subtree.iteritems():
                if is_layout(key):
                    layout = self.layouts_dict[layout_key(key)]
                    _merge(place, layout.get_yaml_tree())
                    continue

                if isinstance(val, basestring):
                    if is_layout(val):
                        layout = self.layouts_dict[layout_key(val)]
                        _merge(_subchapter(place, key), layout.get_yaml_tree())
                        continue

                    old_val = _find_similar(place, key)
                    if old_val is None:
                        place[key] = val
                    else:
                        msg = u'В раздел "{chapter}" для интента "{intent}" уже записано "{old_val}"'.format(
                            chapter=key,
                            intent=val,
                            old_val=old_val.keys() if isinstance(old_val, dict) else old_val
                        )
                        raise DuplicatedSubTree(msg.encode('utf-8'))

                elif isinstance(val, dict):
                    _merge(_subchapter(place, key), val)
                else:
                    msg = u'what the type??? %s' % val
                    raise UserWarning(msg.encode('utf-8'))

        chapters = LexicDict()
        _merge(chapters, self.prj_conf.get_raw_tree())
        return chapters

    # Печать в yaml - для наглядности

    @staticmethod
    def _to_str(line):
        if ':' in line or line.startswith('"'):
            return '"%s"' % line.encode('utf-8').replace('"', '\\"')
        else:
            return line.encode('utf-8')

    @classmethod
    def _yield_yaml_lines(cls, subtree, indent=0):
        prefix = ' ' * indent
        for name, val in subtree.iteritems():
            if isinstance(val, basestring):
                yield '%s%s: %s' % (prefix, cls._to_str(name), val.encode('utf-8'))
            elif isinstance(subtree, dict):
                yield '%s%s:' % (prefix, cls._to_str(name))
                for l in cls._yield_yaml_lines(val, indent + 2):
                    yield l

    def to_yaml(self):
        return '\n'.join(self._yield_yaml_lines(self.chapters))

    # Печать в json - для js кода

    def to_script(self):
        code = self.prj_conf.get_spec_content('script.js', as_unicode=False)
        return code.replace('$intent_tree', json_dumps(self.to_dict()))

    def to_instruction(self, old_instr):
        new_instr = self.prj_conf.get_spec_content('instruction.html')

        if new_instr:
            return new_instr.replace('$intent_tree', self.to_html())
        else:
            parts = old_instr.split(self.HTML_PREFIX)
            if len(parts) == 2:
                return u'{}\n{}'.format(parts[0].strip(),
                                        self.to_html())
        return None

    def to_dict(self):
        def _to_item((idx, (k, v))):
            name_parts = k.split('|', 1)
            name = name_parts[0].strip()
            #if isinstance(v, dict):
            #    name += ' >'
            item = OrderedDict([
                ("name", name),
                ("instruction", name_parts[1].strip() if len(name_parts) == 2 else ""),
                ("key", "item_%s" % idx),
                ("intent", v),
                ("subitems", _to_sub_chapter(v)),
            ])
            if not isinstance(v, basestring):
                del item['intent']
            return item

        def _to_sub_chapter(chapter):
            if isinstance(chapter, basestring):
                return None
            return map(_to_item, enumerate(chapter.iteritems()))

        return OrderedDict([
            ("name", ""),
            ("instruction", ""),
            ("key", "root"),
            ("subitems", _to_sub_chapter(self.chapters)),
        ])

    # Печать в html - для добавления в инструкцию

    def _yield_html_lines(self, mapping, indent=0):
        prefix = ' ' * indent
        yield prefix + '<ul>'

        for key, val in mapping.iteritems():
            name_parts = key.split('|', 1)
            name = name_parts[0].strip()
            is_chapter = isinstance(val, dict)
            if is_chapter:
                yield '%s  <li><b>%s:</b>' % (prefix, name)
                if len(name_parts) == 2:
                    yield '%s      <br/><small>%s</small>' % (prefix, name_parts[1])

                for line in self._yield_html_lines(val, indent + 4):
                    yield line
                yield prefix + '  </li>'
            elif len(name_parts) == 2:
                yield '%s  <li>%s<br/><small>%s</small></li>' % (prefix, name, name_parts[1])
            else:
                yield '%s  <li>%s</li>' % (prefix, name)
        yield prefix + '</ul>'

    HTML_PREFIX = u'<div id="full_intent_tree">' # Используется для патча инструкции

    def to_html(self):
        wrapper = u'{}\n{}\n</div>'
        content = u'\n'.join(self._yield_html_lines(self.chapters))
        return wrapper.format(self.HTML_PREFIX, content)
