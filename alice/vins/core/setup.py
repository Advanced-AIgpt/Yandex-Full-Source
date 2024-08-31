# coding: utf-8
from setuptools import setup, find_packages, Extension
from Cython.Build import cythonize

version = '0.1.1'
long_description = ''
with open('README') as f:
    long_description = f.read()

ext_module = [Extension(
    'vins_core.nlu.knn',
    sources=['vins_core/nlu/knn/knn.pyx'],
    extra_compile_args=["-std=c++11"],
)]

setup(
    name='vins-core',
    version=version,
    description='Voice INterfaces form-filler-dm application',
    long_description=long_description,
    author='Evgeny Volkov',
    author_email='emvolkov@yandex-team.ru',
    packages=find_packages(),
    include_package_data=True,
    ext_modules=cythonize(ext_module, annotate=False),
)
