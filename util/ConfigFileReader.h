//
// Created by chiyuen on 2021/11/3.
//

#ifndef DBSERVERCONNECT_CONFIGFILEREADER_H
#define DBSERVERCONNECT_CONFIGFILEREADER_H
#include "util.h"


class CConfigFileReader{
public:
     CConfigFileReader(const char* file_name);

    ~CConfigFileReader();

    char* GetConfigName(const char* name);

    uint32_t SetConfigValue(const char* name,const char* value);

private:
    void LoadFile(const char* file_name);

    int WriteFile(const char * file_name= nullptr);

    void ParseLine(char* line);

    char *TrimSpace(char* line);

    bool m_load_ok; //是否加载配置文件完毕
    std::string m_config_filename; // 配置文件路径
    std::map<std::string,std::string> m_config_map; // 缓存 存放配置

};


#endif //DBSERVERCONNECT_CONFIGFILEREADER_H
