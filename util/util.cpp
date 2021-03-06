//
// Created by chiyuen on 2021/11/2.
//

#include "util.h"

CStrExplode::CStrExplode(char *str, char seperator) {
    m_item_cnt = 1;
    char *pos  = str;
    while (*pos){
        if (*pos == seperator)
            m_item_cnt++;
        pos++;
    }
    m_item_list = new char* [m_item_cnt];
    char* start  = pos = str;

    int idx = 0;
    while (*pos){
        if (start != pos && *pos == seperator){
            uint32_t len =(uint32_t)(pos -start);
            m_item_list[idx] = new char[len + 1];
            strncpy(m_item_list[idx],start,len);
            m_item_list[idx][len] = '\0';
            idx++;
            start = pos+1;
        }
        pos++;
    }
    uint32_t len =(uint32_t)(pos - start);
    if (len != 0){
        m_item_list[idx] = new char[len + 1];
        strncpy(m_item_list[idx],start,len);
        m_item_list[idx][len] = '\0';
    }
}

CStrExplode::~CStrExplode() {
    for (uint32_t i = 0; i < m_item_cnt; ++i) {
        delete []m_item_list[i];
    }
    delete []m_item_list;
}
