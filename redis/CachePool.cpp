//
// Created by chiyuen on 2021/11/2.
//

#include "CachePool.h"
#define MIN_CACHE_CONN_CNT 2

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
    for (uint32_t i = 0; i < instances_name.GetItemCnt(); ++i) {
        char * pool_name = instances_name.GetItem(i);
        sprintf(host,"%s_host",pool_name);
        sprintf(port,"%s_port",pool_name);
        sprintf(db,"%s_db",pool_name);
        sprintf(maxconncnt,"%s_maxconncnt",pool_name);
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

void CacheManager::RelCacheConn(CacheConn *pCacheConn) {
    if (!pCacheConn){
        return;
    }
    std::map<std::string,CachePool*>::iterator it = m_cache_pool_map.find(pCacheConn->GetPoolName());
    if (it != m_cache_pool_map.end()){
        return it->second->RelCacheConn(pCacheConn);
    }
}

CacheConn *CacheManager::GetCacheConn(const char *pool_name) {
    std::map<std::string,CachePool*>::iterator it = m_cache_pool_map.find(pool_name);
    if (it != m_cache_pool_map.end()){
        return it->second->GetCacheConn();
    } else{
        return nullptr;
    }
}

CachePool::CachePool(const char *pool_name, const char *server_ip, uint32_t server_port, uint32_t db_num,uint32_t max_conn_cnt) {
    m_pool_name = pool_name;
    m_server_ip = server_ip;
    m_server_port = server_port;
    m_db_num = db_num;
    m_max_conn_cnt = max_conn_cnt;
    m_cur_conn_cnt = MIN_CACHE_CONN_CNT;
}

CachePool::~CachePool() {
    //上锁
    m_free_notify.Lock();
    for (std::list<CacheConn*>::iterator it = m_free_list.begin();it != m_free_list.end();++it) {
        CacheConn *pConn = *it;
        delete pConn;
    }
    m_free_list.clear();
    m_cur_conn_cnt = 0;
    m_free_notify.Unlock();
}

int CachePool::Init() {
    for (int i = 0; i < m_cur_conn_cnt; ++i) {
        CacheConn *pConn = new CacheConn(this);
        if (pConn->Init()){
            delete pConn;
            return 1;
        }
        m_free_list.push_back(pConn);
    }
    DLOG(INFO)<< "cache pool  "<<m_pool_name<< " list size "<<m_free_list.size()<<endl;
    return 0;
}

CacheConn *CachePool::GetCacheConn() {
    m_free_notify.Lock();
    while (m_free_list.empty()){
        if (m_cur_conn_cnt >= m_max_conn_cnt){
            m_free_notify.Wait();
        }else{
            CacheConn *pCacheConn = new CacheConn(this);
            int ret = pCacheConn->Init();
            if (ret){
                DLOG(INFO)<< " Init CacheConn failed "<<endl;
                delete pCacheConn;
                pCacheConn = nullptr;
                m_free_notify.Unlock();
            } else{
                m_free_list.push_back(pCacheConn);
                m_cur_conn_cnt++;
                DLOG(INFO)<< " new cache connection "<<m_pool_name<< " conn_cnt "<<m_cur_conn_cnt<<endl;
            }
        }
    }
    CacheConn *pConn = m_free_list.front();
    m_free_list.pop_front();
    m_free_notify.Unlock();
    return pConn;
}

void CachePool::RelCacheConn(CacheConn *pCacheConn) {
    m_free_notify.Lock();
    std::list<CacheConn*>::iterator it = m_free_list.begin();
    for (; it != m_free_list.end(); it++) {
        if (*it == pCacheConn){
            break;
        }
    }
    if (it == m_free_list.end()){
        m_free_list.push_back(pCacheConn);
    }
    m_free_notify.Signal();
    m_free_notify.Unlock();

}

CacheConn::CacheConn(CachePool *pCachePool) {
    m_pCachePool = pCachePool;
    m_pContext = nullptr;
    m_last_connect_time = 0;
}

CacheConn::~CacheConn() {
    if (m_pContext){
        redisFree(m_pContext);
        m_pContext = nullptr;
    }
}

int CacheConn::Init() {
    if (m_pContext)
        return 0;
    uint64_t cur_time = (uint64_t)time(nullptr);
    if (cur_time < m_last_connect_time + 4){
        return 1;
    }
    m_last_connect_time  = cur_time;
    struct timeval timeout = {0,200000};
    m_pContext = redisConnectWithTimeout(m_pCachePool->GetServerIP(),m_pCachePool->GetServerPort(),timeout);
    if (!m_pContext || m_pContext->err){
        if (m_pContext){
            DLOG(INFO)<< "redisConnect failed error with "<<m_pContext->errstr<<endl;
            redisFree(m_pContext);
            m_pContext = nullptr;
        }else{
            DLOG(INFO)<< "redisConnect failed "<<endl;
        }
        return 1;
    }
    redisReply *reply = (redisReply* )redisCommand(m_pContext,"SELECT %d",m_pCachePool->GetDBNum());
    if (reply && (reply->type == REDIS_REPLY_STATUS) && (strncmp(reply->str,"OK",2) == 0)){
        freeReplyObject(reply);
        return 0;
    }else{
        DLOG(INFO)<< " select cache db failed "<<endl;
        return 2;
    }
}

const char *CacheConn::GetPoolName() {
    return m_pCachePool->GetPoolName();
}
