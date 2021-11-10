#include <iostream>
#include <glog/logging.h>
#include <csignal>
#include "redis/CachePool.h"


// 初始化Glog日志
void InitGlog(char *argv[]){
    /**初始化glog日志**/
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_colorlogtostderr = true;


}

int main(int argc, char *argv[]) {
    InitGlog(argv);

    //
    signal(SIGPIPE, SIG_IGN);
    srand(time(NULL));

    CacheManager* pCacheManager = CacheManager::GetInstance();

    if (!pCacheManager){
        DLOG(ERROR)<<" CacheManager init failed "<<std::endl;
        return  -1;
    }

    DLOG(INFO) << "Hello, World!" << std::endl;
    return 0;
}
