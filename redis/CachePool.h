//
// Created by chiyuen on 2021/11/2.
//

#ifndef DBSERVERCONNECT_CACHEPOOL_H
#define DBSERVERCONNECT_CACHEPOOL_H

#include "util.h"
#include "ConfigFileReader.h"
#include "Thread.h"



// redis 缓存池对象
class CachePool;

// redis 连接对象
class CacheConn{
public:
    CacheConn(CachePool* pCachePool);
    virtual ~CacheConn() ;

    //redis 初始化连接和重连操作,类似 mysql_ping
    int Init();

    // 获取redis数据库表连接池名称
    const char* GetPoolName();

    // redis get命令  GET key 获取指定的key值
    std::string get(std::string key);

    // redis set命令 设置指定key的值 注意 一个键最大能存储512MB  SET key value
    std::string set(std::string key,std::string& value);

    // redis setex命令 为指定的key设置值及其过期时间，如果key已经存在,SETEX命令将会替换旧的值
    std::string setex(std::string key,int timeout,std::string value);

    // 批量获取 获取所有(一个或多个)给定key的值   MGET key1 [key2 ....]
    bool mget(const std::vector<std::string> &key,std::map<std::string,std::string>& ret_value);

    //判断一个Key是否存在 EXISTS KEY
    bool isExists(std::string &key);

    // redis hash数据结构

    // 删除一个或多个hash表字段 HDEL key field1[field2]
    uint64_t hdel(std::string key,std::string field);

    // 获取 field对应的value 每个hash可以存储2^32键值对
    std::string hget(std::string key,std::string value);

    //获取在hash表中指定key的所有字段和值
    bool hgetAll(std::string key,std::map<std::string,std::string>& hash);

    //
    uint64_t hset(std::string key,std::string fields,std::string value);

    // 为hash表中的指定字段的整数值加上增量increment
    uint64_t hincrBy(std::string key,std::string field,uint64_t value);

    //将key所存储的值加上给定的增量值
    uint64_t incrBy(std::string key,uint64_t value);

    // 设置hash存储的key与value
    std::string hmset(std::string key,std::map<std::string,std::string>& hash);

    // 获取所有给定字段的值
    bool hmget(std::string key,std::list<std::string>& fields,std::list<std::string>& ret_value);

    // 将key中存储的数字值增一
    uint64_t incr(std::string key);

    // 将key中存储的数字值减一
    uint64_t decr(std::string key);

    // redis list操作
    // 将一个或多个值插入到列表头部
    uint64_t lpush(std::string key,std::string value);

    // 在列表中插入一个或多个列表值
    uint64_t rpush(std::string key,std::string value);

    // 获取列表的长度
    uint64_t llen(std::string key);

    // 获取列表指定范围内的元素
    bool lrange(std::string key,uint64_t start,uint64_t end,std::list<std::string>& ret_value);

private:
    CachePool* m_pCachePool;
    redisContext* m_pContext;
    uint64_t m_last_connect_time;

};

class CachePool{
public:
    CachePool(const char* pool_name,const char* server_ip,uint32_t server_port,uint32_t db_num,uint32_t max_conn_cnt );
    virtual ~CachePool();
    //初始化CachePool
    int Init();
    CacheConn *GetCacheConn();
    void RelCacheConn(CacheConn* pCacheConn);

    const char* GetPoolName(){ return m_pool_name.c_str();}
    const char* GetServerIP(){ return m_server_ip.c_str();}
    uint32_t GetServerPort(){return m_server_port;}
    uint32_t GetDBNum(){return m_db_num;}

private:
    std::list<CacheConn*> m_free_list; // 空闲CachePool
    CThreadNotify m_free_notify;

    std::string m_pool_name;
    std::string m_server_ip; // 指定redis 绑定的主机地址
    uint32_t m_server_port; // 指定redis监听端口
    uint32_t m_db_num;      // 指定数据库的数量
    uint32_t m_max_conn_cnt; //指定同一时间内最大客户端的连接数 maxclient表示不做限制
    uint32_t m_cur_conn_cnt; //当前连接redis数据库数量



};

//redis单例对象
class CacheManager{
public:
    virtual ~CacheManager();

    //创建单例对象指针
    static CacheManager* GetInstance();

    //初始化单例对象指针
    int Init();
    CacheConn* GetCacheConn(const char* pool_name);

    void RelCacheConn(CacheConn* pCacheConn);

private:
    CacheManager();
    std::map<std::string,CachePool*> m_cache_pool_map;

private:
    static CacheManager* s_cache_manager;

};


#endif //DBSERVERCONNECT_CACHEPOOL_H
