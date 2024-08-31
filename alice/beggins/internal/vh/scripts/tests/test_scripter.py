import pytest


import textwrap


from alice.beggins.internal.vh.scripts.scripter import Scripter

import resources.fun_body_simple as fun_body_simple
import resources.fun_body_complex_begin as fun_body_complex_begin
import resources.fun_body_complex_middle as fun_body_complex_middle
import resources.fun_body_complex_end as fun_body_complex_end

import resources.fun_body_zero_args as fun_body_zero_args
import resources.fun_body_one_args as fun_body_one_args
import resources.fun_body_two_args as fun_body_two_args
import resources.fun_body_three_args as fun_body_three_args
import resources.fun_body_four_args as fun_body_four_args


def test_simple():
    expected = textwrap.dedent('''
    def main(v, w, x):
        print('Hello, World!')


    return main(v, w, x)
    ''')
    tested = Scripter.to_string(fun_body_simple.main)
    assert tested.strip() == expected.strip()


def test_complex_begin():
    expected = textwrap.dedent('''
    def hello(v, w, x):
        return 'Hello, World!'


    BAZ = 228


    class Foo:
        pass


    def bar():
        pass


    return hello(v, w, x)
    ''')
    tested = Scripter.to_string(fun_body_complex_begin.hello)
    assert tested.strip() == expected.strip()


def test_complex_middle():
    expected = textwrap.dedent('''
    class Foo:
        pass


    def main(v, w, x):
        return 'Hello, World!'


    def bar():
        pass


    return main(v, w, x)
    ''')
    tested = Scripter.to_string(fun_body_complex_middle.main)
    assert tested.strip() == expected.strip()


def test_complex_end():
    expected = textwrap.dedent('''
    class Foo:
        pass


    def bar():
        pass


    def baz(v, w, x):
        return 'Hello, World!'


    return baz(v, w, x)
    ''')
    tested = Scripter.to_string(fun_body_complex_end.baz)
    assert tested.strip() == expected.strip()


def test_zero_args():
    expected = textwrap.dedent('''
    def zero():
        return 0


    return zero()
    ''')
    tested = Scripter.to_string(fun_body_zero_args.zero)
    assert tested.strip() == expected.strip()


def test_one_args():
    expected = textwrap.dedent('''
    def one(a):
        return a


    return one(v)
    ''')
    tested = Scripter.to_string(fun_body_one_args.one)
    assert tested.strip() == expected.strip()


def test_two_args():
    expected = textwrap.dedent('''
    def two(a, b):
        return a + b


    return two(v, w)
    ''')
    tested = Scripter.to_string(fun_body_two_args.two)
    assert tested.strip() == expected.strip()


def test_three_args():
    expected = textwrap.dedent('''
    def three(a, b, c):
        return a + b + c


    return three(v, w, x)
    ''')
    tested = Scripter.to_string(fun_body_three_args.three)
    assert tested.strip() == expected.strip()


def test_four_args():
    with pytest.raises(Exception):
        Scripter.to_string(fun_body_four_args.four)
