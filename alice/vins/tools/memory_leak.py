# coding: utf-8

from collections import deque

import click
import networkx as nx
from meliae import loader

from IPython import embed


def diff(om1, om2):
    """ New ObjManager that contains only elements from om2 that not present in om1 """
    addr_diff = set(om2.objs) - set(om1.objs)
    return loader.ObjManager({addr: om2[addr] for addr in addr_diff})


def load(filename):
    """ Load meliae dump to ObjectManager """
    om = loader.load(filename)
    om.collapse_instance_dicts()
    om.compute_referrers()
    om.remove_expensive_references()
    om.guess_intern_dict()
    return om


def dump(om, fd):
    """ Dump ObjectManager back to file """
    visited = {0}

    def _dump(obj):
        if obj.address not in visited:
            fd.write(obj.to_json() + '\n')
            visited.add(obj.address)

    for obj in om.objs.itervalues():
        if obj.address not in visited:
            _dump(obj)

            for ref in obj.iter_recursive_refs():
                _dump(ref)


def make_graph(obj):
    """ Create parents graph of ref """
    g = nx.DiGraph()
    visited = set()
    queue = deque()

    queue.append(obj)

    while queue:
        cur = queue.popleft()
        visited.add(cur)

        for parent in cur.p:
            g.add_edge(parent, cur)

            if parent not in visited:
                queue.append(parent)

    return g


def save_graph(gr, filename):
    """ Save graph to svg """
    from networkx.drawing.nx_pydot import to_pydot
    pd = to_pydot(gr)
    pd.write_svg(filename)


def save_ref_as_svg(ref, filename=None):
    """ Save graph of object's parents to file """
    if filename is None:
        filename = '%s_%s.svg' % (ref.type_str, ref.address)

    save_graph(make_graph(ref), filename)
    return filename


@click.command(help='Creates diff of two meliae\'s objects dumps, compute diff and open ipython shell.')
@click.argument('file1', type=click.Path(exists=True))
@click.argument('file2', type=click.Path(exists=True))
def main(file1, file2):
    print 'loading %s' % file1
    om1 = load(file1)
    print
    print 'loading %s' % file2
    om2 = load(file2)
    print
    print 'diff summary:'
    omd = diff(om1, om2)
    print omd.summarize()

    # start ipython
    embed()


if __name__ == '__main__':
    main()
