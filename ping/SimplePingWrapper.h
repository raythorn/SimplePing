//
// Created by Derek Ray on 2017/12/30.
//

#ifndef _SIMPLEPINGWRAPPER_H
#define _SIMPLEPINGWRAPPER_H

#include <string>
#include <pthread.h>

#include "SimplePing.h"

class SimplePingWrapper : public SimplePingDelegate {
public:
    SimplePingWrapper();
    ~SimplePingWrapper();

    static SimplePingWrapper *sharedInstance();
    void ping(std::string host, int count);

    virtual void simplePing(int state, std::string message);
private:
    SimplePing *pinger;
    static SimplePingWrapper *instance;
    static pthread_mutex_t mutex;
};


#endif //_SIMPLEPINGWRAPPER_H
