package org.alipay.idworker;

/**
 * @author Administrator
 */
public interface RandomCodeStrategy {
    void init();

    int prefix();

    int next();

    void release();
}
