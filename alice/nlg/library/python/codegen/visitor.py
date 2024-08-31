# coding: utf-8

from alice.nlg.library.python.codegen import nodes


class NodeVisitor(object):
    permissive = False
    base_node_class = nodes.Node
    visitor_exception = AttributeError

    def visit(self, node, *args, **kwargs):
        visitor = self.get_visitor(node)
        if visitor is not None:
            self.pre_visit(node)
            try:
                return self.alter_result(node, visitor(node, *args, **kwargs))
            finally:
                self.post_visit(node)

    def get_visitor(self, node):
        assert isinstance(node, self.base_node_class)

        for cls in type(node).__mro__:
            method = 'visit_' + cls.__name__
            attr = getattr(self, method, None)
            if attr is not None:
                return attr

        if not self.permissive:
            raise self.visitor_exception(
                'Visitor method not found for node {!r}'.format(node)
            )

    def alter_result(self, node, result):
        """Used to modify the result of a visit according to some
        general rules based on the node's properties.
        """
        return result

    def pre_visit(self, node):
        pass

    def post_visit(self, node):
        pass


class GenericNodeVisitor(object):
    def visit(self, node, *args, **kwargs):
        if not isinstance(node, nodes.Node):
            return

        self.generic_visit(node, *args, **kwargs)
        for field_name in node.fields:
            field = getattr(node, field_name)
            if isinstance(field, list):
                for element in field:
                    self.visit(element, *args, **kwargs)
            else:
                self.visit(field, *args, **kwargs)

    def generic_visit(self, node):
        raise NotImplementedError()
