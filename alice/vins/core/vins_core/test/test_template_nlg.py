# coding: utf-8
from __future__ import unicode_literals

import pytest

from jinja2 import TemplateAssertionError

from vins_core.nlg.template_nlg import TemplateNLG, Template, create_simple_phrases, get_env, create_branched_phrase
from vins_core.dm.form_filler.models import Form


@pytest.fixture
def templates(tmpdir):
    content = """
    {% phrase test %}
      phrase test
    {% endphrase %}

    {% card test %}
      {"card": "test"}
    {% endcard %}

    {% macro test() %}
      hello from macro
    {% endmacro %}

    {% cardtemplate test %}
    {% endcardtemplate %}
    """

    tmpdir.join('test.nlg').write(content)
    return str(tmpdir)


@pytest.fixture
def env(tmpdir):
    return get_env(str(tmpdir))


@pytest.mark.parametrize('from_file', [True, False])
def test_nlg_parse(tmpdir, env, from_file):
    src = """
    {% phrase alarm_was_set %}
      {% chooseline %}
        будильник установлен на {{ form.when }}
        я разбужу тебя в {{ form.when }}
        # комментарий
      {% endchooseline %}
    {% endphrase %}

    {% phrase test %}
      foo
    {% endphrase %}

    {% card test %}
      bar
    {% endcard %}

    {% cardtemplate test %}
    {% endcardtemplate %}
    """
    if from_file:
        f = str(tmpdir.join('file.nlg'))
        open(f, 'w').write(src.encode('utf-8'))
        kwargs = {'filename': 'file.nlg', 'env': env}
    else:
        kwargs = {'string': src}

    result = Template(**kwargs)

    assert result.get_phrase('alarm_was_set') is not None
    assert result.get_phrase('test') is not None
    assert result.get_card('test') is not None
    assert result.get_cardtemplate('test') is not None

    # check that comment ignored
    rendered = {result.render_phrase('alarm_was_set', form={'when': '12'}).text for _ in xrange(1000)}
    assert rendered == {'будильник установлен на 12', 'я разбужу тебя в 12'}


@pytest.mark.parametrize('mode', ["", ", 'no_repeat'", ", 'cycle'"])
def test_chooseline(mode):
    sio = """
    {% phrase alarm_was_set %}
      {% chooseline""" + mode + """ %}
        будильник установлен на {{ form.when }}
        я разбужу тебя в {{ form.when }}
        # комментарий
      {% endchooseline %}
    {% endphrase %}
    """

    f = TemplateNLG()
    f.add_intent('test_intent', sio)

    form = Form.from_dict({
        'name': 'test_intent',
        'slots': [
            {
                'name': 'when',
                'value': '7 утра',
                'type': 'string',
                'optional': True
            }
        ]
    })

    assert f.has_phrase('alarm_was_set', form.name)
    assert not f.has_phrase('unknown_phrase', form.name)
    assert not f.has_phrase('unknown_phrase_2')

    used_replies = []

    def render_phrase():
        return f.render_phrase('alarm_was_set', form, context=dict(used_replies=used_replies)).text

    if mode == "":
        assert render_phrase() == 'будильник установлен на 7 утра'
    elif mode == ", 'no_repeat'":
        assert render_phrase() == 'будильник установлен на 7 утра'
        assert render_phrase() == 'я разбужу тебя в 7 утра'
        assert render_phrase() == ''
    else:
        assert render_phrase() == 'будильник установлен на 7 утра'
        assert render_phrase() == 'я разбужу тебя в 7 утра'
        assert render_phrase() == 'будильник установлен на 7 утра'
        assert render_phrase() == 'я разбужу тебя в 7 утра'


def test_template_nlg_global(tmpdir):
    nlg = """
        {% phrase test_global %}
          {% chooseline %}
            test
          {% endchooseline %}
        {% endphrase %}
        """
    tmpdir.join('global.nlg').write(nlg)
    nlg = TemplateNLG(global_templates=['global.nlg'], templates_dir=str(tmpdir))
    assert nlg.render_phrase('test_global').text == 'test'


def test_template_nlg_global_for_intent(templates):
    nlg = TemplateNLG(global_templates=['test.nlg'], templates_dir=str(templates))
    nlg.add_intent('test_intent', ' ')

    form = Form.from_dict({
        'name': 'test_intent',
        'slots': []
    })

    assert nlg.render_phrase('test', form).text == 'phrase test'
    assert nlg.render_card('test', form) == {'card': 'test'}


def test_nlg_additional_context():
    sio = """
    {% phrase alarm_was_set %}
        будильник установлен на {{ context.block.day }} {{ form.when }}
    {% endphrase %}
    """

    f = TemplateNLG()
    f.add_intent('test_intent', sio)

    form = Form.from_dict({
        'name': 'test_intent',
        'slots': [
            {
                'name': 'when',
                'value': '7 утра',
                'type': 'string',
                'optional': True
            }
        ]
    })

    context = {'block': {'day': 'понедельник'}}

    assert f.render_phrase('alarm_was_set', form, context=context).text == 'будильник установлен на понедельник 7 утра'


def test_maybe_tag():
    template = """
    {% phrase test %}
      {% maybe %}
        abc
      {% endmaybe %}
    {% endphrase %}
    """
    parsed = Template(template)
    expected = {'', 'abc'}
    assert {parsed.render_phrase('test').text for _ in xrange(100)} == expected


def test_maybe_tag_weighted():
    template = """
    {% phrase test %}
      {% maybe 0.2 %}
        abc
      {% endmaybe %}
    {% endphrase %}
    """
    parsed = Template(template)
    stats = {}
    expected = {'abc': 0.2, '': 0.8}
    n = 20000
    for _ in xrange(n):
        res = parsed.render_phrase('test').text
        stats[res] = stats.get(res, 0) + 1

    for k, v in stats.items():
        stats[k] = round(float(v) / n, 2)

    assert stats == expected


def test_chooseitem_tag():
    template = """
    {% phrase test %}
      {% chooseitem %}
        test1
      {% or %}
        {% if 1 == 1 %}
          test2
        {% endif %}
      {% or %}
        {% with a=3 %}
          test{{ a }}
        {% endwith %}
      {% endchooseitem %}
    {% endphrase %}
    """
    parsed = Template(template)
    expected = {'test1', 'test2', 'test3'}
    assert {parsed.render_phrase('test').text for _ in xrange(1000)} == expected


def test_chooseitem_tag_weighted():
    template = """
    {% phrase test %}
      {% chooseitem 0.4 %}
        test1
      {% or 0.6 %}
        {% if 1 == 1 %}
          test2
        {% endif %}
      {% or 3 %}
        {% with a=3 %}
          test{{ a }}
        {% endwith %}
      {% endchooseitem %}
    {% endphrase %}
    """
    parsed = Template(template)
    stats = {}
    expected = {'test1': 0.1, 'test2': 0.15, 'test3': 0.75}
    n = 20000
    for _ in xrange(n):
        res = parsed.render_phrase('test').text
        stats[res] = stats.get(res, 0) + 1

    for k, v in stats.items():
        stats[k] = round(float(v) / n, 2)

    assert stats == expected


def test_outter_definitions():
    template = """
    {% set a=word|capitalize %}

    {% phrase test %}
      test={{ a }}
    {% endphrase %}

    {% phrase test2 %}
       test2={{ a }}
    {% endphrase %}
    """

    parsed = Template(template)
    assert parsed.render_phrase('test', word='ololo').text == 'test=Ololo'
    assert parsed.render_phrase('test2', word='ololo').text == 'test2=Ololo'


def test_ignore_outside_text():
    template = """
    outside text

    {% phrase test %}
      test text
    {% endphrase %}
    """

    parsed = Template(template)
    assert parsed.render_phrase('test').text == 'test text'


def test_voice_text_tags():
    template = """
    {% phrase test1 %}
        {% voice %}
            Hell+o
        {% endvoice %}
        {% text %}
            Hello
        {% endtext %}
        {%vc%}w+orld{%evc%}{%tx%}world{%etx%}
    {% endphrase %}
    {% phrase test2 %}
        {% voice %}
            S+ynonymous t+ags
            hell+o
        {% evc %}
        {% tx %}
            Synonymous tags
            hello
        {% endtext %}
        {%vc%}w+orld{%evc%}{%tx%}world{%etx%}
    {% endphrase %}    """

    parsed = Template(template)

    render_result = parsed.render_phrase('test1', word='ololo')
    assert render_result.voice == 'Hell+o w+orld'
    assert render_result.text == 'Hello world'

    render_result = parsed.render_phrase('test2', word='ololo')
    assert render_result.voice == 'S+ynonymous t+ags hell+o w+orld'
    assert render_result.text == 'Synonymous tags hello world'


def test_nested_voice_text_tags():
    template = """
    {% phrase test %}
        {% voice %}
            Hell+o
            {% text %}
                Hello
            {% endtext %}
        {% endvoice %}
    {% endphrase %}
    """
    with pytest.raises(TemplateAssertionError):
        Template(template)

    template = """
    {% phrase test %}
        {% text %}
            Hell+o
            {% voice %}
                Hello
            {% endvoice %}
        {% endtext %}
    {% endphrase %}
    """

    with pytest.raises(TemplateAssertionError):
        Template(template)

    template = """
    {% phrase test %}
        {% voice %}
            Hell+o
            {% voice %}
                Hello
            {% endvoice %}
        {% endvoice %}
    {% endphrase %}
    """

    with pytest.raises(TemplateAssertionError):
        Template(template)

    template = """
    {% phrase test %}
        {% text %}
            Hell+o
            {% text %}
                Hello
            {% endtext %}
        {% endtext %}
    {% endphrase %}
    """

    with pytest.raises(TemplateAssertionError):
        Template(template)


def test_mixed_voice_text_tags():
    template = """
    {% phrase test %}
        {% voice %}
            Hell+o
        {% text %}
        {% endvoice %}
            Hello
        {% endtext %}
    {% endphrase %}
    """
    with pytest.raises(TemplateAssertionError):
        Template(template)


@pytest.mark.parametrize('from_file', [True, False])
def test_import(templates, tmpdir, from_file):
    nlg = TemplateNLG(templates_dir=templates)

    source = """
    {% nlgimport "test.nlg" %}

    {% phrase template_test %}
      template
    {% endphrase %}
    """

    if from_file:
        f = str(tmpdir.join('file.nlg'))
        open(f, 'w').write(source)

        kwargs = {'filename': 'file.nlg'}
    else:
        kwargs = {'data': source}

    nlg.add_intent('test_intent', **kwargs)

    form = Form.from_dict({
        'name': 'test_intent',
        'slots': []
    })
    assert nlg.has_card('test', 'test_intent')
    assert nlg.has_phrase('test', 'test_intent')
    assert nlg.has_cardtemplate('test', 'test_intent')

    assert nlg.render_phrase('test', form).text == 'phrase test'
    assert nlg.render_card('test', form) == {'card': 'test'}
    assert nlg.render_phrase('template_test', form).text == 'template'


def test_filepath_in_traceback(tmpdir, env):
    template = """
    {% phrase test %}
      {{ 1/0 }}
    {% endphrase %}
    """
    filename = 'ololo.nlg'
    f = tmpdir.join(filename)
    f.write(template)

    t = Template(filename=filename, env=env)

    with pytest.raises(ZeroDivisionError) as exc:
        t.render_phrase('test')

    last_line = exc.traceback[-1]
    assert last_line.path == str(f)


def test_create_simple_phrases():
    phrases = create_simple_phrases(
        phrases=[
            ('phrase1', ['a', 'b']),
            ('phrase2', ['привет'])
        ],
        includes=[
            'test.nlg'
        ]
    )
    assert phrases == """{% nlgimport "test.nlg" %}

{% phrase phrase1 %}
  {% chooseline %}
    a
    b
  {% endchooseline %}
{% endphrase %}

{% phrase phrase2 %}
  {% chooseline %}
    привет
  {% endchooseline %}
{% endphrase %}
"""


def test_create_branched_phrase():
    phrase = create_branched_phrase(
        phrase_id='my_phrase',
        blocks={
            'is_smth': [
                'a', 'b'
            ],
            'is_another': [
                'c', 'd'
            ],
            'else': [
                'e'
            ],
        },
        includes=[
            'test.nlg'
        ]
    )
    assert phrase == """{% nlgimport "test.nlg" %}

{% phrase my_phrase %}
  {% if is_smth() %}
    {% chooseline %}
      a
      b
    {% endchooseline %}
  {% elif is_another() %}
    {% chooseline %}
      c
      d
    {% endchooseline %}
  {% else %}
    {% chooseline %}
      e
    {% endchooseline %}
  {% endif %}
{% endphrase %}
"""


def test_json():
    f = TemplateNLG()

    source = """
    {% from "json_macro.jinja" import json_list %}

    {% card json_test %}
        {% call(item) json_list([1, 2, 3]) %}
            {{ item }}
        {% endcall %}
    {% endcard %}
    """

    f.add_intent('test_intent', source)

    template = f.get_card_template('test_intent', 'json_test')
    assert template.render_card('json_test') == [1, 2, 3]


def test_from_file_list(tmpdir, env):
    t1 = '''
    {% phrase test %}
      test phrase
    {% endphrase %}
    '''

    t2 = '''
    {% card test %}
      {"card": "test"}
    {% endcard %}
    '''

    tmpdir.join('file1').write(t1)
    tmpdir.join('file2').write(t2)

    t = Template.from_file_list(['file1', 'file2'], env)

    assert t.get_phrase('test') is not None
    assert t.get_card('test') is not None


def test_from_empty_file_list(env):
    assert Template.from_file_list([], env) is None


def test_dynamic_phrase():
    src = """
    {% if global_var == 1 %}
      {% phrase test %}
        True
      {% endphrase %}
    {% else %}
      {% phrase test %}
        False
      {% endphrase %}
    {% endif %}
    """

    t = Template(src)

    assert t.render_phrase('test', global_var=1).text == 'True'
    assert t.render_phrase('test', global_var=2).text == 'False'


def test_cyclic_import(tmpdir, templates):
    t1 = '''
    {% nlgimport "test.nlg" %}
    {% nlgimport "t2.nlg" %}
    '''

    t2 = '''
    {% nlgimport "t1.nlg" %}
    {% nlgimport "test.nlg" %}
    '''

    tmpdir.join('t1.nlg').write(t1)
    tmpdir.join('t2.nlg').write(t2)

    nlg = TemplateNLG(templates_dir=str(tmpdir))
    with pytest.raises(TemplateAssertionError) as exc:
        nlg.add_intent('test', filename='t1.nlg')

    assert exc.value.message == '''Cyclic import found.
Traceback:
\tt2.nlg:3
\tt1.nlg:2
\tt2.nlg:3'''


def test_chained_import(tmpdir, env, templates):
    t1 = '''
    {% nlgimport "test.nlg" %}
    '''

    t2 = '''
    {% nlgimport "t1.nlg" %}
    '''

    tmpdir.join('t1.nlg').write(t1)
    tmpdir.join('t2.nlg').write(t2)

    t = Template(filename='t2.nlg', env=env)
    assert t.get_phrase('test') is not None
    assert t.get_card('test') is not None


def test_import_with_target(tmpdir, env, templates):
    t1 = '''
    {% nlgimport "test.nlg" as tst %}

    {% phrase myphrase %}
      {{ tst.test() }}
    {% endphrase %}
    '''

    tmpdir.join('t1.nlg').write(t1)
    t = Template(filename='t1.nlg', env=env)
    assert t.render_phrase('myphrase').text == 'hello from macro'


def test_include(tmpdir, env):
    t1 = '''
    {% set t=global_var %}
    '''

    t2 = '''
    {% include "t1.nlg" %}

    {% phrase test %}
      Phrase with {{ t }}
    {% endphrase %}
    '''

    tmpdir.join('t1.nlg').write(t1)

    t = Template(t2, env=env)
    t.render_phrase('test', global_var='include').text == 'Phrase with include'


def test_nlg_postprocess():
    sio = """
    {% phrase test_phrase %}
        Раз
    {% endphrase %}
    """

    postprocess_filters = """
    {% phrase pp_filter1 %}
        Два {{ current_phrase }}
    {% endphrase %}

    {% phrase pp_filter2 %}
        {{ current_phrase }} Три
    {% endphrase %}
    """

    f = TemplateNLG()
    f.add_intent(None, postprocess_filters)
    f.add_intent('test_intent', sio)

    form = Form.from_dict({
        'name': 'test_intent',
        'slots': []
    })

    result = f.render_phrase('test_phrase', form, postprocess_list=['pp_filter1', 'pp_filter2'])
    assert result.text == 'Два Раз Три'
