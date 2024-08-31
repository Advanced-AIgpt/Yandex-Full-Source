package ru.yandex.quasar.billing.providers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.exception.NotFoundException;

@Component
public class ProviderManager {
    private final Map<String, IProvider> providersByName;
    private final Map<String, IContentProvider> contentProvidersByName;

    public ProviderManager(
            List<IProvider> providers
    ) {
        Map<String, IContentProvider> contentProvidersByNameTmp = new HashMap<>();
        Map<String, IProvider> providersByNameTmp = new HashMap<>();

        for (IProvider provider : providers) {

            String name = provider.getProviderName();

            IProvider oldProvider = providersByNameTmp.put(name, provider);
            if (oldProvider != null) {
                throw new IllegalArgumentException(String.format("More than one content provider with name=%s found",
                        name));
            }

            if (provider instanceof IContentProvider) {
                contentProvidersByNameTmp.put(name, (IContentProvider) provider);
            }
        }
        this.contentProvidersByName = contentProvidersByNameTmp;
        this.providersByName = providersByNameTmp;
    }

    /**
     * Finds content-provider by name, checking it existence
     */
    public IContentProvider getContentProvider(String providerName) {
        IContentProvider contentProvider = contentProvidersByName.get(providerName);

        if (contentProvider == null) {
            throw NotFoundException.unknownProvider(providerName);
        }
        return contentProvider;
    }

    /**
     * Finds provider by name, checking its existence
     *
     * @param providerName provider name
     * @return provider instance
     */
    public IProvider getProvider(String providerName) {
        IProvider provider = providersByName.get(providerName);

        if (provider == null) {
            throw NotFoundException.unknownProvider(providerName);
        }
        return provider;
    }

    public Collection<IContentProvider> getAllContentProviders() {
        return contentProvidersByName.values();
    }

    public Collection<IProvider> getAllProviders() {
        return providersByName.values();
    }

}
