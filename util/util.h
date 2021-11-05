//
// Created by chiyuen on 2021/11/2.
//

#ifndef DBSERVERCONNECT_UTIL_H
#define DBSERVERCONNECT_UTIL_H
#include <map>
#include <set>
#include <hiredis.h>
#include <list>
#include <vector>
#include <iostream>
#include <string>
#include <glog/logging.h>

#define BUFFLEN 256  //逐行读取字符串长度
using std::endl;
using std::cout;


class util {

};

class CStrExplode{
public:
    CStrExplode(char* str,char seperator);
    virtual ~CStrExplode();

    uint32_t GetItemCnt() { return m_item_cnt; }
    char* GetItem(uint32_t idx) { return m_item_list[idx]; }

private:
    uint32_t	m_item_cnt;
    char** 		m_item_list;

};


#endif //DBSERVERCONNECT_UTIL_H
