//
//  ViewController.m
//  SimplePing
//
//  Created by Derek.Ray on 2017/12/31.
//  Copyright © 2017年 Raythorn. All rights reserved.
//

#import "ViewController.h"

#import "SimplePingWrapper.h"

ViewController *controller = nil;

@interface ViewController ()
@property (weak, nonatomic) IBOutlet UITextField *hostedit;
@property (weak, nonatomic) IBOutlet UITextField *countedit;
@property (weak, nonatomic) IBOutlet UITextField *timeoutedit;
@property (weak, nonatomic) IBOutlet UIButton *pingbtn;
@property (weak, nonatomic) IBOutlet UITextView *logview;

- (void)simplePingWithState:(int)state message:(std::string)message;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)simplePingWithState:(int)state message:(std::string)message {
    
    NSString *msg = [NSString stringWithUTF8String:message.c_str()];
    
    __weak ViewController *weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString *text = [weakSelf.logview text];
        if (![text isEqualToString:@""]) {
            text = [text stringByAppendingString:@"\n"];
        }
        
        text = [text stringByAppendingString:msg];
        [weakSelf.logview setText:text];
    });
}

- (IBAction)ping:(id)sender {
    if (sender != _pingbtn) {
        return;
    }
    
    [self.logview setText:@""];
    
    NSString *hostname = [self.hostedit text];
    int count = [[self.countedit text] intValue];
    SimplePingWrapper::sharedInstance()->ping([hostname UTF8String], count);
    controller = self;
}

@end

void _simplePing(int state, std::string message)
{
    [controller simplePingWithState:state message:message];
    NSLog(@"%s", message.c_str());
    if (state == SPNS_PINGRESULT) {
        controller = nil;
    }
}
