//
//  ufoattackAppDelegate.m
//  ufoattack
//
//  Created by Lee Thomason on 10/12/08.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#import "ufoattackAppDelegate.h"
#import "EAGLView.h"

@implementation ufoattackAppDelegate

@synthesize window;
@synthesize glView;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
	glView.animationInterval = 1.0 / 30.0;
	[glView startAnimation];
}


- (void)applicationWillResignActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / 2.0;
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / 30.0;
}


- (void)dealloc {
	[window release];
	[glView release];
	[super dealloc];
}

@end
