//
// Created by chiyuen on 2021/11/5.
//

#include "Thread.h"

CThread::CThread() {
    m_thread_id = 0;
}

CThread::~CThread() {

}

void *CThread::StartRoutine(void *arg) {
    CThread *pThread = (CThread*)arg;
    pThread->OnThreadRun();
    return nullptr;
}

void CThread::StartThread(void) {
    pthread_create(&m_thread_id,nullptr,StartRoutine, this);
}

CEventThread::CEventThread() {
    m_bRunning = false;
}
CEventThread::~CEventThread() {
    StopThread();
}

void CEventThread::StartThread() {
    m_bRunning = true;
    CThread::StartThread();
}

void CEventThread::StopThread() {
    m_bRunning  = false;
}

void CEventThread::OnThreadRun(void) {
    while (m_bRunning){
        OnThreadTick();
    }
}

CThreadNotify::CThreadNotify() {
    pthread_mutexattr_init(&m_mutexattr);
    pthread_mutexattr_settype(&m_mutexattr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex,&m_mutexattr);
    pthread_cond_init(&m_cond,nullptr);
}

CThreadNotify::~CThreadNotify() {
    pthread_mutexattr_destroy(&m_mutexattr);
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}
