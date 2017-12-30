//
// Created by Derek Ray on 2017/12/30.
//

#ifndef _SIMPLEPINGJNI_H
#define _SIMPLEPINGJNI_H

#include <string>

void _simplePingMessage(int state, std::string message);
void _simplePingResult();
void _simplePingFail(int code, std::string error);

#endif //_SIMPLEPINGJNI_H
