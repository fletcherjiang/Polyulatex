package com.alipay.service;

import java.util.List;

import com.alipay.dataobject.Product;
/**
 *
 * @author Mr.Longyx
 * @date 2020/4/19 22:29
 */
public interface ProductService {

	/**
	 * 获取所有产品列表
	 * @return
	 */
	public List<Product> getProducts();
	
	/**
	 * 根据产品ID查询产品详情
	 * @param productId
	 * @return
	 */
	public Product getProductById(String productId);
}
