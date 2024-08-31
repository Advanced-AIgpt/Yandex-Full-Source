# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import os.path


class NluNerMixin(object):
    @classmethod
    def load_from_archive(cls, fst_name, archive, **kwargs):
        tmpdir = archive.get_tmp_dir()
        with archive.nested(fst_name) as arch:
            for fname in arch.list():
                arch.save_by_name(fname, tmpdir)

            return cls(
                fst_name=fst_name,
                fst_path=os.path.join(tmpdir, arch.base),
                **kwargs
            )

    def reload_from_archive(self, name, archive, **kwargs):
        tmpdir = archive.get_tmp_dir()
        with archive.nested(name) as arch:
            for fname in arch.list():
                arch.save_by_name(fname, tmpdir)

            return self.__init__(
                fst_name=name,
                fst_path=os.path.join(tmpdir, arch.base),
                **kwargs
            )

    def parse(self, sample):
        raise NotImplementedError()

    def __call__(self, sample, *args, **kwargs):
        return self.parse(sample)
