//
// Created by chiyuen on 2021/11/2.
//

#ifndef DBSERVERCONNECT_CACHEPOOL_H
#define DBSERVERCONNECT_CACHEPOOL_H

#include "ThreadPool.h"
#include "util.h"
#include "ConfigFileReader.h"


// redis 缓存池对象
class CachePool;

// redis 连接对象
class CacheConn{
public:
    CacheConn() =default;
    ~CacheConn() = default;
    //初始化CacheConn
    int Init();

private:

};

class CachePool{
public:
    CachePool(const char* pool_name,const char* server_ip,uint32_t server_port,uint32_t db_num,uint32_t max_conn_cnt );
    CachePool() = default;
    virtual ~CachePool();
    //初始化CachePool
    int Init();

private:
    std::list<CacheConn*> m_free_list;
    std::string m_pool_name;
    uint32_t m_pool_port;
    uint32_t m_db_num;
    uint32_t m_max_conn_cnt;
    uint32_t m_cur_conn_cnt;


};

//redis单例对象
class CacheManager{
public:
    virtual ~CacheManager();

    //创建单例对象指针
    static CacheManager* GetInstance();

    //初始化单例对象指针
    int Init();

private:
    CacheManager();
    std::map<std::string,CachePool*> m_cache_pool_map;

private:
    static CacheManager* s_cache_manager;

};


#endif //DBSERVERCONNECT_CACHEPOOL_H
