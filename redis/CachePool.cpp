//
// Created by chiyuen on 2021/11/2.
//

#include "CachePool.h"

CacheManager* CacheManager::s_cache_manager = nullptr;

CacheManager::CacheManager() {

}


CacheManager *CacheManager::GetInstance() {
    if (!s_cache_manager){
        s_cache_manager = new CacheManager();
        if (s_cache_manager->Init()){
            delete s_cache_manager;
            s_cache_manager = nullptr;
        }
    }
    return s_cache_manager;
}

int CacheManager::Init() {
    // 读取配置文件
    CConfigFileReader config_file("/home/project/config/dbproxyserver.conf");
    char* cache_instances = config_file.GetConfigName("CacheInstances");
    if (!cache_instances){
        DLOG(ERROR)<<" not configure CacheIntance "<<endl;
        return 1;

    }
    CStrExplode instances_name(cache_instances, ',');
    char host[64]; // redis数据库连接IP
    char port[64];
    char db[64];
    char maxconncnt[64];
    for (int i = 0; i < instances_name.GetItemCnt(); ++i) {
        char * pool_name = instances_name.GetItem(i);
//        DLOG(INFO)<< " pool_name "<<pool_name<<endl;
        sprintf(host,"%s_host",pool_name);
        sprintf(port,"%s_port",pool_name);
        sprintf(db,"%s_db",pool_name);
        sprintf(maxconncnt,"%s_maxconncnt",pool_name);
        DLOG(INFO)<<"pool_name "<<pool_name<<endl;

        char *cache_host  = config_file.GetConfigName(host);
        char *str_cache_port = config_file.GetConfigName(port);
        char *str_cache_db = config_file.GetConfigName(db);
        char *str_max_conn_cnt = config_file.GetConfigName(maxconncnt);
        if (!cache_host || !str_cache_port || !str_cache_db ||!str_max_conn_cnt){
            DLOG(ERROR)<< "not configure cache instance: "<<pool_name<<endl;
            return 2;
        }
        CachePool *pCachePool = new CachePool(pool_name,cache_host,atoi(str_cache_port),atoi(str_cache_db),atoi(str_max_conn_cnt));
        if (pCachePool->Init()){
            DLOG(ERROR)<< "Init cache pool failded "<<endl;
            return 3;
        }
        m_cache_pool_map.insert(std::make_pair(pool_name,pCachePool));
    }

    return 0;

}

CacheManager::~CacheManager() {

}

CachePool::CachePool(const char *pool_name, const char *server_ip, uint32_t server_port, uint32_t db_num,
                     uint32_t max_conn_cnt) {

}

CachePool::~CachePool() {

}

int CachePool::Init() {
    return 0;
}
