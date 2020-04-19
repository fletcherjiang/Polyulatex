package com.alipay.mapper;

import com.alipay.dataobject.Orders;
import com.alipay.dataobject.OrdersExample;
import java.util.List;
import org.apache.ibatis.annotations.Param;
/**
 *
 * @author Mr.Longyx
 * @date 2020/4/19 21:47
 */
public interface OrdersMapper {
    int countByExample(OrdersExample example);

    int deleteByExample(OrdersExample example);

    int deleteByPrimaryKey(String id);

    int insert(Orders record);

    int insertSelective(Orders record);

    List<Orders> selectByExample(OrdersExample example);

    Orders selectByPrimaryKey(String id);

    int updateByExampleSelective(@Param("record") Orders record, @Param("example") OrdersExample example);

    int updateByExample(@Param("record") Orders record, @Param("example") OrdersExample example);

    int updateByPrimaryKeySelective(Orders record);

    int updateByPrimaryKey(Orders record);
}