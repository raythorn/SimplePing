//
//  ViewController.h
//  SimplePing
//
//  Created by Derek.Ray on 2017/12/31.
//  Copyright © 2017年 Raythorn. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, NetState) {
    SPNS_ERROR       = -1,    //General error
    SPNS_DNSUNREACH  = -2,    //DNS server unreachable
    SPNS_HOSTUNREACH = -3,     //Host unreachable
    SPNS_HOSTDOWN    = -4,    //Host is down
    SPNS_HOSTNOTFOUNT= -5,     //Host not found
    SPNS_NETUNREACH  = -6,    //Network unreachable
    SPNS_NETDOWN     = -7,    //Network is down
    SPNS_TIMEOUT     = -8,    //Timeout
    SPNS_NOMEM       = -9,    //Out of memory
    SPNS_IO          = -10,    //IO error
    
    SPNS_OK          = 0,
    
    SPNS_PINGINIT,        //Show ping start message
    SPNS_PING,            //Show ping packet message
    SPNS_PINGSTATISTIC,    //Show ping statistic message
    
    SPNS_PINGRESULT,        //Send ping result, with json string
};

@interface ViewController : UIViewController
@end

