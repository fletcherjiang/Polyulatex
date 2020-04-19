package org.alipay.idworker;
/**
 *
 * @author Mr.Longyx
 * @date 2020/4/19 21:25
 */
public interface WorkerIdStrategy {
    void initialize();

    long availableWorkerId();

    void release();
}
