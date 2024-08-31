package ru.yandex.alice.paskills.common.apphost.spring;

import org.springframework.lang.Nullable;

import ru.yandex.web.apphost.api.request.RequestContext;

public interface ApphostRequestInterceptor {

    /**
     * Interception point before the execution of a handler. Called after
     * HandlerMapping determined an appropriate handler object, but before
     * HandlerAdapter invokes the handler.
     * With this method, each interceptor can decide to abort the execution chain,
     * typically sending an error or writing a custom response.
     */
    default boolean preHandle(ApphostRequestHandlingContext handlingContext, RequestContext request) {
        return true;
    }

    /**
     * Interception point after successful execution of a handler.
     * Called after Handler actually invoked the handler, but before the
     * ApphostServlet processes the response. Can expose additional model objects
     * to RequestContext.
     * With this method, each interceptor can post-process an execution,
     * getting applied in inverse order of the execution chain.
     *
     * <p>The default implementation is empty.
     */
    default void postHandle(ApphostRequestHandlingContext handlingContext, RequestContext request) {

    }


    /**
     * Callback after completion of request processing, that is, after calling the handler.
     * Will be called on any outcome of handler execution, thus allows
     * for proper resource cleanup.
     * <p>Note: Will only be called if this interceptor's {@code preHandle}
     * method has successfully completed and returned {@code true}!
     * <p>As with the {@code postHandle} method, the method will be invoked on each
     * interceptor in the chain in reverse order, so the first interceptor will be
     * the last to be invoked.
     */
    default void afterCompletion(ApphostRequestHandlingContext handlingContext, RequestContext request,
                                 @Nullable Exception ex) {

    }

}
