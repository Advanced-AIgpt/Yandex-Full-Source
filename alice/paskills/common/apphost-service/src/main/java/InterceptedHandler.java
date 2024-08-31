package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.List;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.web.apphost.api.request.RequestContext;

class InterceptedHandler implements Consumer<RequestContext> {

    private final String path;
    private final Consumer<RequestContext> delegate;
    private final List<ApphostRequestInterceptor> interceptors;

    private static final Logger logger = LogManager.getLogger();


    InterceptedHandler(String path,
                       Consumer<RequestContext> delegate,
                       List<ApphostRequestInterceptor> interceptors
    ) {
        this.path = path;
        this.delegate = delegate;
        this.interceptors = interceptors;
    }

    @Override
    public void accept(RequestContext requestContext) {
        logger.trace("Handling apphost request for path: {} Guid: {} Ruid: {}",
                path, requestContext.getGuid(), requestContext.getRuid()
        );

        ApphostRequestHandlingContext handlingContext = new ApphostRequestHandlingContext(path);
        HandlerExecutionChain chain = new HandlerExecutionChain();

        Exception ex = null;
        try {
            if (!chain.applyPreHandle(handlingContext, requestContext)) {
                return;
            }

            delegate.accept(requestContext);

            chain.applyPostHandle(handlingContext, requestContext);
        } catch (Exception e) {
            ex = e;
            // rethrow so that apphost handle exception itself
            throw e;
        } finally {
            chain.triggerAfterCompletion(handlingContext, requestContext, ex);
        }
    }

    private class HandlerExecutionChain {

        private int interceptorIndex = -1;

        boolean applyPreHandle(ApphostRequestHandlingContext handlingContext, RequestContext requestContext) {
            for (int i = 0; i < interceptors.size(); i++) {
                ApphostRequestInterceptor interceptor = interceptors.get(i);
                try {
                    if (!interceptor.preHandle(handlingContext, requestContext)) {
                        triggerAfterCompletion(handlingContext, requestContext, null);
                        return false;
                    }

                    interceptorIndex = i;
                } catch (Exception e) {
                    logger.error("Failed to perform preHandle in class " + interceptor.getClass().getName(), e);
                    // if some interceptors fail the whole request should be failed
                    throw e;
                }
            }
            return true;
        }

        void applyPostHandle(ApphostRequestHandlingContext handlingContext, RequestContext requestContext) {
            for (int i = interceptors.size() - 1; i >= 0; i--) {
                var interceptor = interceptors.get(i);
                try {
                    interceptor.postHandle(handlingContext, requestContext);
                } catch (Exception e) {
                    logger.error("Failed to perform postHandle in class " + interceptor.getClass().getName(), e);
                    throw e;
                }
            }
        }


        void triggerAfterCompletion(ApphostRequestHandlingContext handlingContext, RequestContext requestContext,
                                    @Nullable Exception ex) {
            if (!interceptors.isEmpty()) {
                for (int i = interceptorIndex; i >= 0; i--) {
                    var interceptor = interceptors.get(i);
                    try {
                        interceptor.afterCompletion(handlingContext, requestContext, ex);
                    } catch (Exception e) {
                        logger.error(
                                "Failed to perform afterCompletion in class " + interceptor.getClass().getName(),
                                e);
                    }
                }
            }
        }

        @Override
        public String toString() {
            return "HandlerExecutionChain for [" + path + "] and " + interceptors.size() + " interceptors";
        }
    }


}
