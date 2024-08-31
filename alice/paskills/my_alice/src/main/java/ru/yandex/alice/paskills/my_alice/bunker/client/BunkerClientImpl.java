package ru.yandex.alice.paskills.my_alice.bunker.client;

import java.net.URI;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import lombok.Data;
import lombok.NonNull;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.converter.ByteArrayHttpMessageConverter;
import org.springframework.http.converter.StringHttpMessageConverter;
import org.springframework.http.converter.json.GsonHttpMessageConverter;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

public class BunkerClientImpl implements BunkerClient {
    private static final Logger logger = LogManager.getLogger();

    private final RestTemplate webClient;
    private final String apiUri;
    private final String defaultNodeVersion;

    BunkerClientImpl(RestTemplate webClient, String bunkerApi, String defaultNodeVersion) {
        this.webClient = webClient;
        this.apiUri = bunkerApi;
        this.defaultNodeVersion = defaultNodeVersion;
    }

    BunkerClientImpl(String bunkerApi, String defaultNodeVersion) {
        this.webClient = new RestTemplate(List.of(
                new ByteArrayHttpMessageConverter(),
                new StringHttpMessageConverter(),
                new GsonHttpMessageConverter()
        ));
        this.apiUri = bunkerApi;
        this.defaultNodeVersion = defaultNodeVersion;
    }

    @Override
    public <T> Optional<NodeContent<T>> get(String nodePath, Class<T> responseType) {
        return get(nodePath, defaultNodeVersion, ParameterizedTypeReference.forType(responseType));
    }

    @Override
    public <T> Optional<NodeContent<T>> get(String nodePath, ParameterizedTypeReference<T> responseType) {
        return get(nodePath, defaultNodeVersion, responseType);
    }

    @Override
    public <T> Optional<NodeContent<T>> get(String nodePath, String version, Class<T> responseType) {
        return get(nodePath, defaultNodeVersion, ParameterizedTypeReference.forType(responseType));
    }

    @Override
    public <T> Optional<NodeContent<T>> get(
            String nodePath,
            String version,
            ParameterizedTypeReference<T> responseType
    ) {
        try {
            logger.debug("New request: get node={} version={}", nodePath, version);
            URI uri = UriComponentsBuilder.fromUriString(apiUri)
                    .path("/v1/cat")
                    .queryParam("node", nodePath)
                    .queryParam("version", version)
                    .build()
                    .toUri();

            var response = webClient.exchange(uri, HttpMethod.GET, null, responseType);
            logger.info("Got node content: node={} version={}", nodePath, version);

            if (response.getStatusCodeValue() != 200) {
                throw new ResourceAccessException(String.format("Bad http code: %s",
                        response.getStatusCode().toString()));
            }

            MediaType mediaType = response.getHeaders().getContentType();
            if (mediaType == null) {
                throw new ResourceAccessException("Empty Content-Type header");
            }

            return Optional.of(new NodeContent<>(mediaType, response.getBody()));
        } catch (Exception e) {

            logger.warn("Failed to get node content", e);
            return Optional.empty();
        }
    }

    @Override
    public Map<String, NodeInfo> tree(String rootNodePath) {
        return tree(rootNodePath, defaultNodeVersion);
    }

    @Override
    public Map<String, NodeInfo> tree(String rootNodePath, String version) {
        try {
            logger.debug("New request: tree node={} version={}", rootNodePath, version);
            URI uri = UriComponentsBuilder.fromUriString(apiUri)
                    .path("/v1/tree")
                    .queryParam("node", rootNodePath)
                    .queryParam("version", version)
                    .build()
                    .toUri();

            var response = webClient.exchange(
                    uri,
                    HttpMethod.GET,
                    null,
                    new ParameterizedTypeReference<List<RawNodeInfo>>() {
                    }
            );
            logger.info("Got tree result: node={} version={}", rootNodePath, version);

            var list = response.getBody();
            if (list == null) {
                throw new ResourceAccessException("Response is empty");
            }

            Map<String, NodeInfo> result = new HashMap<>();
            for (RawNodeInfo node : list) {
                if (node == null || Strings.trimToNull(node.getFullName()) == null) {
                    continue;
                }

                result.put(node.getFullName(), node.getNodeInfo());
            }

            return result;
        } catch (Exception e) {

            logger.warn("Failed to get node content", e);
            return Map.of();
        }
    }

    @Data
    @NonNull
    private static class RawNodeInfo {
        private final String fullName;
        @Nullable
        private final String mime;

        NodeInfo getNodeInfo() {
            MediaType mediaType = null;
            if (mime != null) {
                try {
                    mediaType = MediaType.parseMediaType(mime);
                } catch (Exception ignore) {
                }
            }

            return new NodeInfo(fullName, mediaType);
        }
    }
}
