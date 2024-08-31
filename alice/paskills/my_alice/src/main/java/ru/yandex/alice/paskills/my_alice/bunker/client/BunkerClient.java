package ru.yandex.alice.paskills.my_alice.bunker.client;

import java.util.Map;
import java.util.Optional;

import org.springframework.core.ParameterizedTypeReference;

public interface BunkerClient {
    <T> Optional<NodeContent<T>> get(String nodePath, Class<T> responseType);

    <T> Optional<NodeContent<T>> get(String nodePath, ParameterizedTypeReference<T> responseType);

    <T> Optional<NodeContent<T>> get(String nodePath, String version, Class<T> responseType);

    <T> Optional<NodeContent<T>> get(String nodePath, String version, ParameterizedTypeReference<T> responseType);

    Map<String, NodeInfo> tree(String rootNodePath);

    Map<String, NodeInfo> tree(String rootNodePath, String version);
}
