#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals

import hashlib
from utils.nirvana.op_caller import get_current_workflow_url

from api import ProjectRequester
from std_prj_js import JsConstructor


class StdProject(ProjectRequester):
    """Стандартный проект"""
    def __init__(self, prj_id=None, **kwargs):
        super(StdProject, self).__init__(prj_id=prj_id, **kwargs)
        # TODO: Если prj_id задан, то prj_name и skill_id можно вычислить запросами в Толоку
        self.prj_name = None
        self.skill_id = None

    types_map = {
        # Отображение пользовательского типа в тип для input/output спецификаций
        'image': 'url',
        'audio': 'url',
        'video': 'url',
        'selector': 'string',
    }

    def _make_prj_props(self, title, instructions,
                        inp_spec, out_spec,
                        metadata,
                        public_description='',
                        private_description='',
                        custom_markup=None,
                        custom_script=None,
                        custom_styles=None):
        props = self._make_base_prj_props(title, instructions, metadata, public_description, private_description)

        def to_type(field):
            t = field['type']
            return self.types_map.get(t, t)

        if inp_spec is not None:
            props['task_spec']['input_spec'] = {f['name']: {'type': to_type(f)}
                                                for f in inp_spec}

        if out_spec is not None:
            props['task_spec']['output_spec'] = {f['name']: {'type': to_type(f)}
                                                 for f in out_spec}

        if custom_markup:
            props['task_spec']['view_spec']['markup'] = custom_markup
        elif inp_spec and out_spec:  # TODO: Что, если у нас update только одной части спецификации?
            props['task_spec']['view_spec'].update(self._make_view_spec(inp_spec, out_spec))
        if custom_script:
            props['task_spec']['view_spec']['script'] = custom_script
        if custom_styles:
            props['task_spec']['view_spec']['styles'] = custom_styles
        return props

    # PROJECT MARKUP

    _string_fmt = '{task}\n<p>{{{{{field[name]}}}}}</p>'.format

    _img_fmt = '{task}\n{{{{img src={field[name]}}}}}'.format

    _audio_html_fmt = ('{task}<br/>\n'
                       '<audio id="{field[name]}_player" src={{{{{field[name]}}}}} controls controlsList="nodownload">'
                       'Невозможно воспроизвести</audio><br/>').format

    _video_html_fmt = ('{task}<br/>\n'
                       '<video id="{field[name]}_player" src={{{{{field[name]}}}}} controls controlsList="nodownload">'
                       'Невозможно воспроизвести</video><br/>').format

    _bool_fmt = ('{task}\n{{{{field type="checkbox" name="{field[name]}"'
                 ' label="{field[label]}" class="block"}}}}').format

    _choice_fmt = ('{task}\n{{{{field type="radio" name="{field[name]}"'
                   ' value="{choice[value]}" label="{choice[label]}" hotkey="{idx}"}}}}').format

    _label_fmt = '{task}\n{field[label]}'.format

    _input_fmt = ('{task}\n{field[label]}\n'
                  '{{{{field type="input" name="{field[name]}" width="100%"}}}}').format

    def _make_view_spec(self, inp_spec, out_spec):
        task = ''
        js = JsConstructor()
        for field in inp_spec:
            if field.get('label'):
                task = self._label_fmt(task=task, field=field)

            if field['type'] in ('string', 'float', 'integer'):
                task = self._string_fmt(task=task, field=field)
            elif field['type'] == 'image':
                task = self._img_fmt(task=task, field=field)
            elif field['type'] == 'audio':
                task = self._audio_html_fmt(task=task, field=field)
                js.add_field(field)
            elif field['type'] == 'video':
                task = self._video_html_fmt(task=task, field=field)
                js.add_field(field)
            else:
                raise UserWarning('Unknown input field type %s' % field)

        for field in out_spec:
            if field['type'] == 'boolean':
                task = self._bool_fmt(task=task, field=field)

            elif field['type'] == 'selector':
                if field.get('label'):
                    task = self._label_fmt(task=task, field=field)

                for idx, choice in enumerate(field['choices'], start=1):
                    task = self._choice_fmt(task=task, field=field, choice=choice, idx=idx)

            elif field['type'] in ('string', 'float', 'integer'):
                task = self._input_fmt(task=task, field=field)

            else:
                raise UserWarning('Unknown output field type %s' % field)
        return {'markup': task, 'script': js.to_code()}

    def _make_base_prj_props(
            self, title, instructions, metadata,
            public_description, private_description
    ):
        # if self.prj_id is not None:
        #    return self.get_prj_props()
        url = get_current_workflow_url()
        if url is None:
            comment = 'Создан через api скриптом StdProject\n\n%s' % private_description
        else:
            comment = 'Создан через api скриптом StdProject\nиз %s\n\n%s' % (url, private_description)

        return {
            'public_name': title,
            'public_description': public_description,
            'public_instructions': instructions,
            'private_comment': comment,
            'assignments_issuing_type': 'AUTOMATED',
            'task_spec': {
                'input_spec': {},
                'output_spec': {},
                'view_spec': {
                    'assets': {
                        "script_urls": ["$TOLOKA_ASSETS/js/toloka-handlebars-templates.js"],
                    },
                    'markup': '',
                    'script': '',
                    'styles': '',
                    'settings': {},  # мб 'showSkip': False?
                },
            },
            'metadata': metadata,
        }

    def create_prj(self, props=None, **kwargs):
        if props is None:
            props = self._make_prj_props(**kwargs)
        response = super(StdProject, self).create_prj(props)
        self.props = props
        self.prj_name = kwargs['title']
        return response

    def update_prj(self, new_props=None, **kwargs):
        if new_props is None:
            new_props = self._make_prj_props(**kwargs)
        return super(StdProject, self).update_prj(new_props)

    def get_prj_name_and_instructions_hash(self):
        project_props = self.get_prj_props()
        project_instructions = project_props['public_instructions']
        hash = hashlib.sha1((self.prj_name.encode('utf-8') + project_instructions)).hexdigest()
        return hash

    # Навыки

    def get_or_create_accuracy_skill(self):
        if self.skill_id is not None:
            raise ValueError('Skill already created, id={}'.format(self.skill_id))

        if self.prj_id is None:
            raise ValueError('Create (or get) project first!')

        if self.prj_name is None:
            self.prj_name = self.get_prj_props()['public_name']

        hash = self.get_prj_name_and_instructions_hash()
        skill_name = 'Auto generated skill for StdProject id {} - точность (hash {})'.format(self.prj_id, hash)

        skills_with_such_name = self.fetch_list('skills', {'name': skill_name})['items']

        if len(skills_with_such_name) > 1:
            raise ValueError(
                'There are {} skills with name `{}`. Expected to have at most 1.'.format(
                    len(skills_with_such_name), skill_name
                )
            )

        # Skill already exists
        if len(skills_with_such_name) == 1:
            skill_properties = skills_with_such_name[0]
        else:
            props = {
                'name': skill_name,
                'private_comment': 'Auto generated for StdProject id=%s' % self.prj_id,
                'hidden': False,
                'training': False,
            }
            skill_properties = self.send('skills', props)
        self.skill_id = skill_properties['id']
        return skill_properties

    def get_skill_url(self):
        return self._get_url('requester/quality/skill', self.skill_id)
