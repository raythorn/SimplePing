//
// Created by Derek Ray on 2017/12/30.
//

#include "SimplePingWrapper.h"
#include "../android/app/src/main/cpp/SimplePingJni.h"

using namespace std;

SimplePing SimplePingWrapper::pinger;

SimplePingWrapper::SimplePingWrapper()
{
    pinger = nullptr;
}

SimplePingWrapper::~SimplePingWrapper()
{
}

void SimplePingWrapper::ping(string host, int count, int timeout)
{
    if (nullptr == pinger) {
        pinger = new SimplePing;
    }

    pinger->ping(host, count, timeout);
}

void SimplePingWrapper::simplePingMessage(int state, string message)
{
    _simplePingMessage(state, message);
}

void SimplePingWrapper::simplePingResult(ping_result_t *result)
{
    _simplePingResult();
}

void SimplePingWrapper::simplePingFail(int code, string error)
{
    _simplePingFail(code, error);
}