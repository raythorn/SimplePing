//
// Created by Derek Ray on 2017/12/30.
//

#include "SimplePingWrapper.h"
using namespace std;

SimplePingWrapper *SimplePingWrapper::instance;
pthread_mutex_t SimplePingWrapper::mutex;

SimplePingWrapper::SimplePingWrapper()
{
    pinger = nullptr;
    instance = nullptr;
    mutex = PTHREAD_MUTEX_INITIALIZER;
}

SimplePingWrapper::~SimplePingWrapper()
{
    if (pinger) delete pinger;
    pinger = nullptr;
}

SimplePingWrapper* SimplePingWrapper::sharedInstance()
{
    if (nullptr == instance) {
        pthread_mutex_lock(&mutex);
        if (nullptr == instance) {
            instance = new SimplePingWrapper;
        }
        pthread_mutex_unlock(&mutex);
    }

    return instance;
}

void SimplePingWrapper::ping(string host, int count)
{
    if (nullptr == pinger) {
        pinger = new SimplePing();
        pinger->setDelegate(this);
    }

    pinger->ping(host, count);
}

void SimplePingWrapper::simplePing(int state, string message)
{
    extern void _simplePing(int, string);
    _simplePing(state, message);
}
