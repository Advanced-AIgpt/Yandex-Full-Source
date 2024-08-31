package ru.yandex.alice.library.routingdatasource;

import java.util.stream.Stream;

import org.aopalliance.intercept.MethodInterceptor;
import org.springframework.aop.framework.ProxyFactoryBean;
import org.springframework.beans.BeansException;
import org.springframework.beans.factory.config.BeanPostProcessor;
import org.springframework.core.annotation.AnnotationUtils;
import org.springframework.stereotype.Component;
import org.springframework.util.ReflectionUtils;

@Component
public class WithDataSourceBeanPostProcessor implements BeanPostProcessor {

    @Override
    public Object postProcessAfterInitialization(Object bean, String beanName) throws BeansException {
        var hasClassAnnotation = AnnotationUtils.findAnnotation(bean.getClass(), WithDataSource.class) != null;
        var hasMethodAnnotation = Stream.of(ReflectionUtils.getAllDeclaredMethods(bean.getClass()))
                .anyMatch(method -> AnnotationUtils.findAnnotation(method, WithDataSource.class) != null);

        var hasAnnotation = hasClassAnnotation || hasMethodAnnotation;
        if (!hasAnnotation) {
            return bean;
        }

        var proxyFactoryBean = new ProxyFactoryBean();

        proxyFactoryBean.setTarget(bean);
        proxyFactoryBean.setProxyTargetClass(true);


        proxyFactoryBean.addAdvice((MethodInterceptor) invocation -> {
            var withDataSource = AnnotationUtils.findAnnotation(invocation.getMethod(), WithDataSource.class);
            if (withDataSource == null) {
                withDataSource = AnnotationUtils.findAnnotation(bean.getClass(), WithDataSource.class);
            }

            if (withDataSource == null) {
                return invocation.proceed();
            }

            var oldValue = DatasourceTypeContextHolder.getDatasourceType();
            DatasourceTypeContextHolder.set(withDataSource.value());
            try {
                return invocation.proceed();
            } finally {
                if (oldValue != null) {
                    DatasourceTypeContextHolder.set(oldValue);
                } else {
                    DatasourceTypeContextHolder.clear();
                }
            }
        });

        return proxyFactoryBean.getObject();
    }
}
