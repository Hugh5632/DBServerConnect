//
// Created by chiyuen on 2021/11/3.
//

#include "ConfigFileReader.h"

CConfigFileReader::CConfigFileReader(const char* file_name) {
    LoadFile(file_name);
}

CConfigFileReader::~CConfigFileReader() {

}

char *CConfigFileReader::GetConfigName(const char* name) {
    if (!m_load_ok)
        return nullptr;
    char *value = nullptr;
    std::map<std::string,std::string>::iterator it = m_config_map.find(name);
    if (it != m_config_map.end()){
        value = (char*)it->second.c_str();
    }
    return value;
}

char *CConfigFileReader::SetConfigValue() {
    return nullptr;
}

void CConfigFileReader::LoadFile(const char *file_name) {
        m_config_filename.clear();
        m_config_filename.append(file_name);
        FILE *fp = fopen(file_name,"r");
        if (!fp){
            DLOG(INFO)<< "can not open  "<<file_name<<errno<<std::endl;
            return;
        }
        char buf[BUFFLEN];
        for(;;){
            //逐行读取字符串
            char* p = fgets(buf,BUFFLEN,fp);
            if (!p)
                break;
            size_t len = strlen(buf); // 数组长度
            // remove \n at the end
            if (buf[len-1] == '\n')
                buf[len-1] = 0;

            char *ch = strchr(buf,'#');
            if (ch)
                ch = 0;
            if (strlen(buf) == 0)
                continue;
            ParseLine(buf);
        }
        fclose(fp);
        m_load_ok = true;

}

int CConfigFileReader::WriteFile(const char *file_name) {
    return 0;
}

void CConfigFileReader::ParseLine(char *line) {
    char* p = strchr(line,'=');
    if (!p)
        return;
    *p = 0;

    char* key = TrimSpace(line);
    char* value = TrimSpace(p+1);
    if (key && value){
        m_config_map.insert(std::make_pair(key,value));
    }

}

char *CConfigFileReader::TrimSpace(char* name) {
    // remove starting space or tab
    char* start_pos = name;
    while ((*start_pos == ' ') || (*start_pos == '\t')){
        start_pos++;
    }
    if(strlen(start_pos) == 0){
        return nullptr;
    }
    // remove ending space or tab
    char * end_pos = start_pos+strlen(name)-1;
    while ((*end_pos == ' ')||(*end_pos == '\t')){
        *end_pos = 0;
        end_pos--;
    }
    int len = (int)(end_pos - start_pos) + 1;
    if (len <= 0)
        return nullptr;

    return start_pos;
}
