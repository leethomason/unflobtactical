//
//  EAGLView.m
//  ufoattack
//
//  Created by Lee Thomason on 10/12/08.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//



#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"

#define USE_DEPTH_BUFFER 1

// A class extension to declare private methods
@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) NSTimer *animationTimer;

- (BOOL) createFramebuffer;
- (void) destroyFramebuffer;

@end


@implementation EAGLView

@synthesize context;
@synthesize animationTimer;
@synthesize animationInterval;


// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}


//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder {
    
    if ((self = [super initWithCoder:coder])) {
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        
        if (!context || ![EAGLContext setCurrentContext:context]) {
            [self release];
            return nil;
        }
        
        animationInterval = 1.0 / 60.0;
		game = 0;
		startTime = CFAbsoluteTimeGetCurrent();
		isDragging = false;
		isZooming = false;
    }
    return self;
}


- (void)drawView {
    
    [EAGLContext setCurrentContext:context];
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glViewport(0, 0, backingWidth, backingHeight);
    
	double timeD = CFAbsoluteTimeGetCurrent() - startTime;
	GameDoTick( game, (unsigned)(timeD * 1000.0) );
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
}


- (void)layoutSubviews {
    [EAGLContext setCurrentContext:context];
    [self destroyFramebuffer];
    [self createFramebuffer];
    [self drawView];
}


- (BOOL)createFramebuffer {
    
    glGenFramebuffersOES(1, &viewFramebuffer);
    glGenRenderbuffersOES(1, &viewRenderbuffer);
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
    
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
    
    if (USE_DEPTH_BUFFER) {
        glGenRenderbuffersOES(1, &depthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    }
    
    if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
	game = NewGame( backingWidth, backingHeight );
	GameRotate( game, 1 );
	
    return YES;
}


- (void)destroyFramebuffer {
    DeleteGame( game );
	game = 0;
	
    glDeleteFramebuffersOES(1, &viewFramebuffer);
    viewFramebuffer = 0;
    glDeleteRenderbuffersOES(1, &viewRenderbuffer);
    viewRenderbuffer = 0;
    
    if(depthRenderbuffer) {
        glDeleteRenderbuffersOES(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
}


- (void)startAnimation {
    self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}


- (void)stopAnimation {
    self.animationTimer = nil;
}


- (void)setAnimationTimer:(NSTimer *)newTimer {
    [animationTimer invalidate];
    animationTimer = newTimer;
}


- (void)setAnimationInterval:(NSTimeInterval)interval {
    
    animationInterval = interval;
    if (animationTimer) {
        [self stopAnimation];
        [self startAnimation];
    }
}


- (void)dealloc {
    
    [self stopAnimation];
    
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
}


- (float)calcLength:(CGPoint)p0 p1:(CGPoint)p1
{  
	float x = p0.x - p1.x;
	float y = p0.y - p1.y;
	return sqrt( x*x + y*y );
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = backingHeight-1-location.y;
	int touchCount = [[event allTouches] count];
	
	// Handle errors.
	if ( isDragging ) {
		GameDrag( game, GAME_DRAG_END, lastDrag.x, lastDrag.y );
		isDragging = false;
	}
	if ( isZooming ) {
		isZooming = false;
	}

	// Process touch
	if ( touchCount == 1 ) {
		GameDrag( game, GAME_DRAG_START, x, y );
		isDragging = true;
		lastDrag.x = x;
		lastDrag.y = y;
	}
	else if ( touchCount == 2 ) {
		UITouch *touch1 = [[[event allTouches] allObjects] objectAtIndex:0];
		UITouch *touch2 = [[[event allTouches] allObjects] objectAtIndex:1];
		
		float distance = [self calcLength:[touch1 locationInView:self] p1:[touch2 locationInView:self]];	
		
		GameZoom( game, GAME_ZOOM_START, distance );
		isZooming = true;
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = backingHeight-1-location.y;
	int touchCount = [[event allTouches] count];
	
	if ( isDragging ) {
		if ( touchCount == 1 ) {
			// Still dragging!
			GameDrag( game, GAME_DRAG_MOVE, x, y );
			lastDrag.x = x; lastDrag.y = y;
		}
		else if ( touchCount == 2 ) {
			// Stop and switch to zoom.
			GameDrag( game, GAME_DRAG_END, lastDrag.x, lastDrag.y );
			isDragging = false;

			UITouch *touch1 = [[[event allTouches] allObjects] objectAtIndex:0];
			UITouch *touch2 = [[[event allTouches] allObjects] objectAtIndex:1];
			
			float distance = [self calcLength:[touch1 locationInView:self] p1:[touch2 locationInView:self]];	
			
			GameZoom( game, GAME_ZOOM_START, distance );
			isZooming = true;
		}
	}
	else if ( isZooming ) {
		if ( touchCount < 2 ) {
			// Lost a connection in there. End the zoom.
			isZooming = false;
		}
		else if ( touchCount == 2 ) { 
			// The normal zoom case.
			UITouch *touch1 = [[[event allTouches] allObjects] objectAtIndex:0];
			UITouch *touch2 = [[[event allTouches] allObjects] objectAtIndex:1];
			
			float distance = [self calcLength:[touch1 locationInView:self] p1:[touch2 locationInView:self]];	
			
			GameZoom( game, GAME_ZOOM_MOVE, distance );
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = backingHeight-1-location.y;
	int touchCount = [[event allTouches] count];

	if ( isZooming && touchCount < 2 ) {
		isZooming = false;
	}
	if ( isDragging && touchCount == 1 ) {
		int x = location.x;
		int y = backingHeight-1-location.y;
		
		GameDrag( game, GAME_DRAG_END, x, y );
		isDragging = false;
	}
	
	int tapCount = [touch tapCount];
	if ( tapCount > 0 ) {
		GameTap( game, tapCount, x, y );
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	GameInputCancelled( game );
	isDragging = false;
	isZooming = false;
}

@end
