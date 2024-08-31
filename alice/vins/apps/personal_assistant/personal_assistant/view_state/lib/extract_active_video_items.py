#!/usr/bin/env python
# -*- coding: utf-8 -*-


def walk_vertex(vertex, get_neighbors, callback):
    callback(vertex)
    for neighbor in get_neighbors(vertex):
        walk_vertex(neighbor, get_neighbors, callback)


def walk_vertices(vertices, get_neighbors, callback):
    for vertex in vertices:
        walk_vertex(vertex, get_neighbors, callback)


def extract_active_video_items(view_state):
    res = []
    walk_vertices(
        view_state.get('sections', []),
        lambda vertex: vertex.get('items', []) + vertex.get('sections', []) if vertex.get('active') else [],
        lambda vertex: res.append(vertex) if vertex.get('type') == 'video' and vertex.get('active') else [])

    return res
