//
// Created by chiyuen on 2021/11/23.
//

#ifndef DBSERVERCONNECT_DBPOOL_H
#define DBSERVERCONNECT_DBPOOL_H

#include "util.h"
#include "ConfigFileReader.h"
#include "Thread.h"
#include <mysql.h>

class CDBPool;
class CDBConn{
public:
    CDBConn(CDBPool* pDBPool);
    virtual ~CDBConn();
    int Init();

private:
    CDBPool* m_pDBPool;
    MYSQL* m_mysql;


};

class CDBPool{
public:
    CDBPool();
    virtual ~CDBPool();
    int Init();
private:
    std::string m_pool_name;
    std::string m_db_server_ip;
    uint16_t m_db_server_port;
    std::string m_username;
    std::string m_password;
    std::string m_db_name;
    uint32_t m_db_cur_conn_cnt;
    uint32_t m_db_max_conn_cnt;
    std::list<CDBConn*> m_free_list;
    CThreadNotify m_free_notify;

};

class DBMannager{
public:
    virtual ~DBMannager();
    static DBMannager* getInstance();
    int Init();

private:
    DBMannager();

    static DBMannager* s_db_manager;
    std::map<std::string ,CDBPool*> m_dbpool_map; //存储MySql连接池,每个池子里面有多个MySql连接对象


};


#endif //DBSERVERCONNECT_DBPOOL_H
