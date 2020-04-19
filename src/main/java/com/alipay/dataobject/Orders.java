package com.alipay.dataobject;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.ToString;

import java.io.Serializable;
import java.util.Date;

/**
 * @author Administrator
 */
@Data
@AllArgsConstructor
@NoArgsConstructor
@ToString
public class Orders implements Serializable {
    private String id;

    private String orderNum;

    private String orderStatus;

    private String orderAmount;

    private String paidAmount;

    private String productId;

    private Integer buyCounts;

    private Date createTime;

    private Date paidTime;

}