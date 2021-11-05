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

    char* SetConfigValue();

private:
    void LoadFile(const char* file_name);

    int WriteFile(const char * file_name);

    void ParseLine(char* line);

    char *TrimSpace(char* line);

    bool m_load_ok;
    std::string m_config_filename;
    std::map<std::string,std::string> m_config_map;

};


#endif //DBSERVERCONNECT_CONFIGFILEREADER_H
