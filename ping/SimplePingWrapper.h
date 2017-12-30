//
// Created by Derek Ray on 2017/12/30.
//

#ifndef _SIMPLEPINGWRAPPER_H
#define _SIMPLEPINGWRAPPER_H

#include <string>

#include "SimplePing.h"

class SimplePingWrapper : public SimplePingDelegate {
public:
    SimplePingWrapper();
    ~SimplePingWrapper();

    static void ping(std::string host, int count, int timeout);

    virtual void simplePingMessage(int state, std::string message);
    virtual void simplePingResult(ping_result_t *result);
    virtual void simplePingFail(int code, std::string error);
private:
    static SimplePing *pinger;
};


#endif //_SIMPLEPINGWRAPPER_H
