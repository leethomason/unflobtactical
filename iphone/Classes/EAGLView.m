//
//  EAGLView.m
//  ufoattack
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
		self.multipleTouchEnabled = YES;
        
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
    
	game = NewGame( backingWidth, backingHeight, 1 );
	//GameRotate( game, 1 );
	
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

-(void)dumpTouch:(UITouch*) touch
{
#ifdef _DEBUG
	const char* phase = "";
	switch ([touch phase]) {
		case UITouchPhaseBegan:			phase = "UITouchPhaseBegan";			break;
		case UITouchPhaseMoved:			phase = "UITouchPhaseMoved";			break;
		case UITouchPhaseStationary:	phase = "UITouchPhaseStationary";			break;
		case UITouchPhaseEnded:			phase = "UITouchPhaseEnded";			break;
		case UITouchPhaseCancelled:		phase = "UITouchPhaseCancelled";			break;
		default:
			break;
	}
	NSLog(@"phase='%s'", phase );	
#endif
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = location.y;
	int touchCount = [[event allTouches] count];
	isMoving = false;
	
	// Handle errors.
	if ( isDragging ) {
		GameDrag( game, GAME_DRAG_END, lastDrag.x, lastDrag.y );
		isDragging = false;
	}
	if ( isZooming ) {
		isZooming = false;
	}

	NSLog(@"Began x=%d y=%d isZooming=%d isDragging=%d touchCount=%d", x, y, isZooming?1:0, isDragging?1:0, touchCount );
	//NSLog(@"Began phase=%d", [touch phase] );
	[self dumpTouch:touch];
	
	// Process touch
	if ( touchCount == 1 ) {
		// NSLog(@"  dragging" );
		GameDrag( game, GAME_DRAG_START, x, y );
		isDragging = true;
		lastDrag.x = x;
		lastDrag.y = y;
	}
	else if ( touchCount == 2 ) {
		UITouch *touch1 = [[[event allTouches] allObjects] objectAtIndex:0];
		UITouch *touch2 = [[[event allTouches] allObjects] objectAtIndex:1];
		
		float distance = [self calcLength:[touch1 locationInView:self] p1:[touch2 locationInView:self]];	
		//NSLog(@"  zooming distance=%.2f", distance );
		
		GameZoom( game, GAME_ZOOM_START, distance );
		isZooming = true;

		//NSArray *touchesArray = [[event allTouches] allObjects];
		CGPoint p0 = [touch1 locationInView:self];
		CGPoint p1 = [touch2 locationInView:self];
		orbitStart = -atan2( p0.x-p1.x, p0.y-p1.y )*180.0f/3.14159f;
		//NSLog(@"orbitStart=%.2f", orbitStart );
		GameCameraRotate( game, GAME_ROTATE_START, orbitStart ); 
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	[self dumpTouch:touch];
	
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = location.y;
	int touchCount = [[event allTouches] count];
	isMoving = true;
	
	//NSLog(@"Moved isZooming=%d isDragging=%d touchCount=%d", isZooming?1:0, isDragging?1:0, touchCount );
	
	if ( isDragging ) {
		if ( touchCount == 1 ) {
			// Still dragging!
			GameDrag( game, GAME_DRAG_MOVE, x, y );
			lastDrag.x = x; lastDrag.y = y;
			//NSLog(@"  dragging" );
		}
		else if ( touchCount > 1 ) {
			// Stop and switch to zoom.
			GameDrag( game, GAME_DRAG_END, lastDrag.x, lastDrag.y );
			isDragging = false;

			UITouch *touch1 = [[[event allTouches] allObjects] objectAtIndex:0];
			UITouch *touch2 = [[[event allTouches] allObjects] objectAtIndex:1];
			
			float distance = [self calcLength:[touch1 locationInView:self] p1:[touch2 locationInView:self]];	
			//NSLog(@"  zooming distance=%.2f", distance );
			
			GameZoom( game, GAME_ZOOM_START, distance );
			isZooming = true;
			
			NSArray *touchesArray = [[event allTouches] allObjects];
			CGPoint p0 = [[touchesArray objectAtIndex:0] locationInView:self];
			CGPoint p1 = [[touchesArray objectAtIndex:1] locationInView:self];
			orbitStart = -atan2( p0.x-p1.x, p0.y-p1.y )*180.0f/3.14159f;
			//NSLog(@"orbitStart=%.2f", orbitStart );
			GameCameraRotate( game, GAME_ROTATE_START, orbitStart ); 
		}
	}
	else if ( isZooming ) {
		if ( touchCount < 2 ) {
			// Lost a connection in there. End the zoom.
			isZooming = false;
			//NSLog(@"  Zoom lost" );
		}
		else { 
			// The normal zoom case.
			NSArray *touchesArray = [[event allTouches] allObjects];
			CGPoint p0 = [[touchesArray objectAtIndex:0] locationInView:self];
			CGPoint p1 = [[touchesArray objectAtIndex:1] locationInView:self];
			
			float distance = [self calcLength:p0 p1:p1];	
			
			GameZoom( game, GAME_ZOOM_MOVE, distance );
			//NSLog(@"  zoom distance=%.2f", distance );
			float r = -atan2( p0.x-p1.x, p0.y-p1.y )*180.0f/3.14159f;
			GameCameraRotate( game, GAME_ROTATE_MOVE, r-orbitStart ); 
			//NSLog(@"r=%.2f", r );
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches anyObject];
	[self dumpTouch:touch];
	
	CGPoint location = [touch locationInView:self];
	int x = location.x;
	int y = location.y;
	int touchCount = [[event allTouches] count];

	//NSLog(@"End isZooming=%d isDragging=%d touchCount=%d", isZooming?1:0, isDragging?1:0, touchCount );
	
	if ( isZooming && touchCount < 2 ) {
		isZooming = false;
	}
	if ( isDragging && touchCount == 1 ) {
		int x = location.x;
		int y = location.y;
		
		GameDrag( game, GAME_DRAG_END, x, y );
		isDragging = false;
	}
	
	int tapCount = [touch tapCount];
	if ( tapCount > 0 && !isMoving) {
		GameTap( game, tapCount, x, y );
	}
	if ( touchCount == 1 ) {
		isMoving = false;
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	GameInputCancelled( game );
	isDragging = false;
	isZooming = false;
}

@end
