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

    const char* GetPoolName();

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
