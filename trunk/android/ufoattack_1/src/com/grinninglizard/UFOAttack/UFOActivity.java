
package com.grinninglizard.UFOAttack;

import java.io.File;
import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

//import org.metalev.multitouch.controller.*;
//import org.metalev.multitouch.controller.MultiTouchController.MultiTouchObjectCanvas;
//import org.metalev.multitouch.controller.MultiTouchController.PointInfo;
//import org.metalev.multitouch.controller.MultiTouchController.PositionAndScale;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
//import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

public class UFOActivity extends Activity  {
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // On the main thread, before we fire off the render thread:
        setWritePaths();
        loadUFOAssets();
       
        mGLView = new DemoGLSurfaceView(this);
        setContentView(mGLView);
    }

     @Override
    protected void onPause() {
        super.onPause();
        mGLView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLView.onResume();
    }
    
    protected void onDestroy() {
    	super.onDestroy();
    	mGLView.destroy();
    }
    
    private void loadUFOAssets() 
    {
    	// Get the path to the application resource.
	    String apkFilePath = null;
	    PackageManager packMgmr = getPackageManager();
	    try
	    {
	    	ApplicationInfo ppInfo = packMgmr.getApplicationInfo( "com.grinninglizard.UFOAttack", 0 );
	        apkFilePath = ppInfo.sourceDir;
	    }
	    catch( NameNotFoundException e){}
	    
	    try {
	    	Resources r = getResources();
	    	AssetFileDescriptor afd = r.openRawResourceFd( R.raw.uforesource );
	   		long offset = afd.getStartOffset();
	        long fileSize = afd.getLength();
	        UFORenderer.nativeResource( apkFilePath, offset, fileSize );
	        afd.close();
	    }
	    catch( IOException e) {
	    	System.out.println( e.toString() );
	    }
	}
    
    private void setWritePaths() 
    {
    	File file = this.getFilesDir();
    	if ( file != null ) {
    		CrashLogger logger = new CrashLogger( file.getAbsolutePath() );		// send crash logs
    		UFORenderer.nativeSavePath( file.getAbsolutePath() );
    	}
    }
   
    private DemoGLSurfaceView mGLView;

    static {
        System.loadLibrary("ufoattack");
    }
}


class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener 
{
	ScaleListener( DemoGLSurfaceView view ) {
		super();
		mView = view;
	}
	
    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        //Log.v( "UFOATTACK", "Scale factor=" + (1.0f-detector.getScaleFactor()) );
        mView.zoom( 1.0f - detector.getScaleFactor() );
        return true;
    }
    private DemoGLSurfaceView mView;
}


/*
class PinchPlane
{
	public float x;
	public float y;
	public float scale;
	public PinchPlane() {
		init();
	}
	
	public void init() {
		x = 0;
		y = 0;
		scale = 1;
	}
}
*/


class DemoGLSurfaceView extends GLSurfaceView { 	//implements MultiTouchObjectCanvas<PinchPlane>  { 

	public DemoGLSurfaceView(Context context) {
        super(context);
        mRenderer = new UFORenderer();
        setRenderer(mRenderer);
        
        requestFocusFromTouch();
        setFocusableInTouchMode( true );
        
        //mScaleDetector = new ScaleGestureDetector(context, new ScaleListener( this ));
        //mPinchPlane = new PinchPlane();
        //mMultiTouchController = new MultiTouchController<PinchPlane>(this);
        mScaleDetector = new ScaleGestureDetector(context, new ScaleListener( this ));
    }
	
	// @Override
	public void onPause() {
		super.onPause();
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_PAUSE, 0, 0, 0, 0 ) );
	}
	
	// @Override
	public void onResume() {
		super.onResume();
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_RESUME, 0, 0, 0, 0 ) );
	}
	
	public void destroy() {
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_DESTROY, 0, 0, 0, 0 ) );
	}
	
	public void zoom( float delta ) {
		// Once zooming kicks in, have to stop moving. Bug in battlescene 
		// can't handle zoom & pan at the same time. The drag is computed from
		// a relative starting coordinate that conflicts with the zoom height
		// getting changed.
		// It would be nice to fix this restriction.
//		if ( multiMode == MODE_MULTI ) {
			queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_ZOOM, GAME_ZOOM_PINCH, 0, 0, delta ) );
//		}
	}

	//private ScaleGestureDetector mScaleDetector;
	//private static final int INVALID_POINTER_ID = -1;
	//private int mActivePointerID = INVALID_POINTER_ID;
	//private int lastX, lastY;									

	private static final int GAME_TAP_DOWN		= 0;
	private static final int GAME_TAP_MOVE		= 1;
	private static final int GAME_TAP_UP		= 2;
	private static final int GAME_TAP_CANCEL	= 3;
	private static final int PANNING			= 0x0100;
	
	private static final int GAME_ZOOM_DISTANCE = 0;
	private static final int GAME_ZOOM_PINCH 	= 1;
	
	
    private UFORenderer mRenderer;
    //private PinchPlane mPinchPlane;
    //private MultiTouchController<PinchPlane> mMultiTouchController;
    private ScaleGestureDetector mScaleDetector;
    
    //private static final int MODE_NONE = 0;
    //private static final int MODE_ONE  = 1;
    //private static final int MODE_MULTI = 2;
	//private int multiMode = MODE_NONE;
	private boolean needToSendUpOrCancel = false;
	
	@Override
	public boolean onTrackballEvent(MotionEvent event) {
		
		//Log.v("UFOATTACK", "dy=" + event.getX() );
		if ( event.getX() != 0 )
			queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_ZOOM, GAME_ZOOM_DISTANCE, 0, 0, -event.getX() ) );
		if ( event.getY() != 0 )
			queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_ROTATE, 0, 0, 0, event.getY()*90.0f ) );
		return true;
	}
	
    @Override
    public boolean onTouchEvent(MotionEvent event) {

    	// Feed info to our helper class:
        int action = event.getAction();
        int type = -1;
        int x = (int)event.getX();
        int y = (int)(getHeight() - event.getY());
        
    	if ( event.getPointerCount() > 1 && needToSendUpOrCancel ) {
   			queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, GAME_TAP_CANCEL, x, y, 0 ) );
   			needToSendUpOrCancel = false;
    	}
 
        mScaleDetector.onTouchEvent(event);

        if ( event.getPointerCount() == 1 ) {
	        switch ( action ) {
		    	case MotionEvent.ACTION_DOWN:
		        	type = GAME_TAP_DOWN;
		        	needToSendUpOrCancel = true;
		        	break;
		        	
		        case MotionEvent.ACTION_UP:
		        	if ( needToSendUpOrCancel ) {
		        		type = GAME_TAP_UP;
		        		needToSendUpOrCancel = false;
		        	}
		        	break;
		        	        	
		        case MotionEvent.ACTION_CANCEL:
		        	if ( needToSendUpOrCancel ) {
		        		type = GAME_TAP_CANCEL;
		        		needToSendUpOrCancel = false;
		        	}
		        	break;
		        	
		        case MotionEvent.ACTION_MOVE:
		        	if ( needToSendUpOrCancel ) {
		        		type = GAME_TAP_MOVE;
		        	}
		        	break;
	        }
	        if ( type >= 0 ) {
	        	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, type, x, y, 0 ) );
	        	return true;
	        }
        }
       	return super.onTouchEvent(event);
    }
/*
	@Override
	public PinchPlane getDraggableObjectAtPoint(PointInfo touchPoint) {
		return mPinchPlane;
	}

	@Override
	public void getPositionAndScale(PinchPlane obj, PositionAndScale objPosAndScaleOut) {
		objPosAndScaleOut.set( 0, 0, true, 1.0f, false, 1, 1, false, 0);
		obj.init();
	}

	@Override
	public boolean setPositionAndScale(PinchPlane obj, PositionAndScale newObjPosAndScale, PointInfo touchPoint) {
		if (    multiMode == MODE_MULTI 
			 && newObjPosAndScale.getScale() != obj.scale ) 
		{
	    	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_ZOOM, 0, 0, 0, obj.scale - newObjPosAndScale.getScale() ) );
	    	obj.scale = newObjPosAndScale.getScale();
		}
		if (    multiMode == MODE_MULTI 
		     && (newObjPosAndScale.getXOff() != obj.x || newObjPosAndScale.getYOff() != obj.y )) 
		{
	    	//queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, GAME_TAP_MOVE | PANNING, (int)newObjPosAndScale.getXOff(), (int)newObjPosAndScale.getYOff(), 0 ) );
	    	obj.x = newObjPosAndScale.getXOff();
	    	obj.y = newObjPosAndScale.getYOff();
		}
		return true;
	}

	@Override
	public void selectObject(PinchPlane obj, PointInfo touchPoint) {
		if ( multiMode == MODE_MULTI && obj == null ) {
	    	//queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, GAME_TAP_UP | PANNING, (int)mPinchPlane.x, (int)mPinchPlane.y, 0 ) );
	    	Log.v("UFOATTACK", "multi mode none" );
			multiMode = MODE_NONE;
		}
	}
*/
	
/*    
   //  @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    	int sendKey = 0;
    	int GAME_HK_NEXT_UNIT	=		0x0001;
    	int GAME_HK_PREV_UNIT	= 		0x0002;
    	//int GAME_HK_ROTATE_CCW	=		0x0004;
    	//int GAME_HK_ROTATE_CW	=		0x0008;

    	switch ( keyCode ) {
    	case KeyEvent.KEYCODE_DPAD_LEFT:
    		sendKey = GAME_HK_PREV_UNIT;
    		break;
    	case KeyEvent.KEYCODE_DPAD_RIGHT:
    		sendKey = GAME_HK_NEXT_UNIT;
    		break;
    	default:
			break;
    	}

    	if ( sendKey != 0 ) {
        	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_HOTKEY, sendKey, 0, 0, 0 ) );
            return true;
        }
    	else {
    		return super.onKeyDown(keyCode, event);
    	}
    }
 */
}


/**
 * The renderer runs on a different thread than the UI. (<vent>Seems needlessly complex - why isn't the driver
 * on a separate thread and the renderer on the main thread?</vent>). Can't call it directly. No evidence it
 * is thread safe, and the underlying c++ code is NOT thread safe. Any communication to the renderer needs
 * to be through the RenderEvent class. 
 *
 */
final class RendererEvent implements Runnable
{
	public static final int TYPE_PAUSE		= 1;
	public static final int TYPE_RESUME 	= 2;
	public static final int TYPE_DESTROY	= 3;
	public static final int TYPE_TAP 		= 100;
	public static final int TYPE_HOTKEY 	= 101;
	public static final int TYPE_ZOOM		= 200;
	public static final int TYPE_ROTATE		= 201;
	
	private UFORenderer renderer;
	private int type;
	private int action;
	private int x;
	private int y;
	private float val;
	
	RendererEvent( UFORenderer renderer, int type, int action, int x, int y, float v ) 
	{
		this.renderer = renderer;
		this.type = type;
		this.action = action;
		this.x = x;
		this.y = y;
		this.val = v;
	}
	
	public void run() 
	{
		switch( type ) {
			case TYPE_PAUSE:
				renderer.pause();
				break;
			case TYPE_RESUME:
				renderer.resume();
				break;
			case TYPE_DESTROY:
				renderer.destroy();
				break;
			case TYPE_TAP:
				renderer.tap( action, (float)x, (float)y );
				Log.v("UFOATTACK", "Tap action=" + action + " x=" + (int)x + " y=" + (int)y );
				break;
			case TYPE_HOTKEY:
				renderer.hotKey( action );
				break;
			case TYPE_ZOOM:
				renderer.zoom( action, val );
			case TYPE_ROTATE:
				renderer.rotate( val );
			default:
				break;
		}
	}
}


/* The renderer is on a separate thread. (oops.) Really
 * does need to be private, and communication is over
 * messages from queueEvent().
 */
class UFORenderer implements GLSurfaceView.Renderer {
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    	Log.v("UFOJAVA", "onSurfaceCreated");
        nativeInit();
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {
    	Log.v("UFOJAVA", "onSurfaceChanged");
        nativeResize(w, h);
    }

    public void onDrawFrame(GL10 gl) {
    	//Log.v("UFOJAVA", "onDrawFrame");
        nativeRender();
    }
    
    public void pause() {
    	nativePause( 1 );
    }
    
    public void resume() {
    	nativePause( 0 );
    }
    
    public void destroy() {
    	nativeDone();
    }
    
    public void tap( int action, float x, float y ) {
    	nativeTap( action, x, y );
    }
    
    public void hotKey( int key ) {
    	Log.v( "UFOJAVA", "sending key=" + key );
    	nativeHotKey( key );
    }
    
    public void zoom( int style, float delta ) {
    	//Log.v("UFOATTACK", "zoom=" + delta );
    	nativeZoom( style, delta );
    }
    
    public void rotate( float delta ) {
    	nativeRotate( delta );
    }

    public static native void nativeResource( String path, long offset, long len );
    public static native void nativeSavePath( String path );
    
    private static native void nativeResize(int w, int h);
    private static native void nativeRender();

    private static native void nativeInit();
    private static native void nativePause( int pause );
    private static native void nativeDone();
    
    private static native void nativeTap( int action, float x, float y );
    private static native void nativeHotKey( int key );
    
    private static native void nativeZoom( int style, float delta );
    private static native void nativeRotate( float delta );
}
