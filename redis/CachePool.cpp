//
// Created by chiyuen on 2021/11/2.
//
/**
 * redis 返回的数据类型 redisReply.type字段
 * REDIS_REPLY_STRING 1 字符串
 * REDIS_REPLY_ARRAY  2 数组，多个Reply 通过element数组以及element数组的大小访问
 * REDIS_REPLY_INTEGER 3 整形
 * REDIS_REPLY_NIL 4 空，没有数据
 * REDIS_REPLY_STATUS 5 状态, str字符串以及len
 * REDIS_REPLY_ERROR 6 错误 同STATUS
 *
 */

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
    for (uint32_t i = 0; i < m_cur_conn_cnt; ++i) {
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
    // 连接redis 若出错redisContext.err会设置为1 redisContext.errstr会包含描述错误信息
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
    // 同步执行redis命令 %d 传入数据 要求有size_t 指定长度
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

std::string CacheConn::get(std::string key) {
    std::string value;
    if (Init()){
        return value;
    }
    redisReply *reply = (redisReply*)redisCommand(m_pContext,"GET %s",value.c_str());
    if (!reply){
        DLOG(INFO)<< " redisCommand failed："<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return value;
    }
    if (reply->type == REDIS_REPLY_STRING){
        value.append(reply->str,reply->len);
    }
    freeReplyObject(reply);
    return value;
}

std::string CacheConn::set(std::string key, std::string &value) {
    std::string ret_value;
    if (Init()){
        return ret_value;
    }
    redisReply *reply =(redisReply*) redisCommand(m_pContext,"SET %s %s",key.c_str(),value.c_str());
    if (!reply){
        DLOG(INFO)<< " redisCommand failed "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return ret_value;
    }
    ret_value.append(reply->str,reply->len);
    freeReplyObject(reply);
    return ret_value;
}

std::string CacheConn::setex(std::string key, int timeout, std::string value) {
    std::string ret_value;
    if (Init()){
        return ret_value;
    }
    redisReply *reply =(redisReply*) redisCommand(m_pContext,"SETEX %s %d %s",key.c_str(),timeout,value.c_str());
    if (!reply){
        DLOG(INFO)<< " redisCommand failed "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return ret_value;
    }
    ret_value.append(reply->str,reply->len);
    freeReplyObject(reply);
    return ret_value;
}

bool CacheConn::mget(const std::vector<std::string> &keys, std::map<std::string, std::string> &ret_value) {
    if (Init()){
        return false;
    }
    if (keys.empty()){
        return false;
    }
    std::string strKey;
    bool bFirst = true;
    for (std::vector<std::string>::const_iterator it  = keys.begin();it != keys.end();++it) {
        if (bFirst){
            bFirst = false;
            strKey = *it;
        }else{
            strKey +=" "+ *it;
        };
    }
    if (strKey.empty()){
        return false;
    }
    strKey = "MGET" + strKey;
    redisReply *reply = (redisReply*)redisCommand(m_pContext,strKey.c_str());
    if (!reply){
        DLOG(INFO)<<" redisCommand failed  "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return false;
    }
    if (reply->type == REDIS_REPLY_ARRAY){
        for (size_t i = 0; i < reply->elements; ++i) {
            redisReply* child_reply = reply->element[i];
            if (child_reply->type == REDIS_REPLY_STRING){
                ret_value[keys[i]] = child_reply->str;
            }
        }
    }
    freeReplyObject(reply);
    return true;
}

bool CacheConn::isExists(std::string &key) {
    if (Init()){
        return false;
    }
    redisReply *reply = (redisReply*) redisCommand(m_pContext,"EXISTS %s",key.c_str());
    if (!reply){
        DLOG(INFO)<< "redisCommand failed "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        return false;
    }
    uint64_t ret_value = reply->integer;
    freeReplyObject(reply);
    if (ret_value == 0){
        return false;
    }else {
        return true;
    }
}

uint64_t CacheConn::hdel(std::string key, std::string field) {
    if (Init()){
        return 0;
    }
    redisReply *reply = (redisReply*)redisCommand(m_pContext,"HDEL %s %s",key.c_str(),field.c_str());
    if (!reply){
        DLOG(INFO)<<" redisCommand failed %s "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return 0;
    }
    uint64_t ret_value = reply->integer;
    freeReplyObject(reply);
    return ret_value;
}

std::string CacheConn::hget(std::string key, std::string value) {
    std::string ret_value;
    if (Init()){
        return ret_value;
    }
    redisReply *reply = (redisReply*)redisCommand(m_pContext,"HGET %s %s",key.c_str(),value.c_str());
    if (!reply){
        DLOG(INFO)<<" redisCommand failed %s "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return ret_value;
    }
    if (reply->type == REDIS_REPLY_STRING){
        ret_value.append(reply->str,reply->len);
    }
    freeReplyObject(reply);
    return ret_value;
}

bool CacheConn::hgetAll(std::string key, std::map<std::string, std::string> &ret_value) {
    if (Init()){
        return false;
    }
    redisReply *reply = (redisReply*)redisCommand(m_pContext,"HGETALL %s",key.c_str());
    if (!reply){
        DLOG(INFO)<<" redisCommand failed %s "<<m_pContext->errstr<<endl;
        redisFree(m_pContext);
        m_pContext = nullptr;
        return false;
    }
    // 处理数据 隔一拿一
    if ((reply->type == REDIS_REPLY_ARRAY) && (reply->elements % 2 == 0)){
        for (size_t i = 0; i < reply->elements; i += 2) {
            redisReply *field_reply = reply->element[i];
            redisReply *value_reply = reply->element[i+1];

            std::string field(field_reply->str,field_reply->len);
            std::string value(value_reply->str,value_reply->len);
            ret_value.insert(std::make_pair(field,value));
        }
    }
    freeReplyObject(reply);
    return true;
}

uint64_t CacheConn::hget(std::string key, std::string fields, std::string value) {
    return 0;
}

uint64_t CacheConn::hincrBy(std::string key, std::string field, uint64_t value) {
    return 0;
}

uint64_t CacheConn::incrBy(std::string key, uint64_t value) {
    return 0;
}

std::string CacheConn::hmset(std::string key, std::map<std::string, std::string> &hash) {
    return std::string();
}

bool CacheConn::hmget(std::string key, std::list<std::string> &fields, std::list<std::string> &ret_value) {
    return false;
}

uint64_t CacheConn::incr(std::string key) {
    return 0;
}

uint64_t CacheConn::decr(std::string key) {
    return 0;
}

uint64_t CacheConn::lpush(std::string key, std::string value) {
    return 0;
}

uint64_t CacheConn::rpush(std::string key, std::string value) {
    return 0;
}

uint64_t CacheConn::llen(std::string key) {
    return 0;
}

bool CacheConn::lrange(std::string key, uint64_t start, uint64_t end, std::list<std::string> &ret_value) {
    return false;
}

