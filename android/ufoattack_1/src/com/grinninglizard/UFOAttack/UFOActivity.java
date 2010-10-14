
package com.grinninglizard.UFOAttack;

import java.io.File;
import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

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
    		logger = new CrashLogger( file.getAbsolutePath() );		// send crash logs
    		//Thread thread = new Thread( logger );
    		//thread.start();
    		UFORenderer.nativeSavePath( file.getAbsolutePath() );
    	}
    }
   
    private DemoGLSurfaceView mGLView;
    private CrashLogger logger = null;

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


class DemoGLSurfaceView extends GLSurfaceView  { 

	public DemoGLSurfaceView(Context context) {
        super(context);
        mRenderer = new UFORenderer();
        setRenderer(mRenderer);
        
        requestFocusFromTouch();
        setFocusableInTouchMode( true );
        
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
		if ( mActivePointerID != INVALID_POINTER_ID ) {
			queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, 3, lastX, lastY, 0 ) );	// cancel
			mActivePointerID = INVALID_POINTER_ID;
		}
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_ZOOM, 0, 0, 0, delta ) );
	}

	private ScaleGestureDetector mScaleDetector;
	private static final int INVALID_POINTER_ID = -1;
	private int mActivePointerID = INVALID_POINTER_ID;
	private int lastX, lastY;									
	
    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	// Feed info to our helper class:
    	mScaleDetector.onTouchEvent(event);
    	
        int action = event.getAction();
        
        for( int i=0; i<event.getPointerCount(); ++i ) {

            int pointerIndex = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
        	int pointerID = event.getPointerId(pointerIndex);

            int x = (int)event.getX(pointerID);
            int y = (int)(getHeight() - event.getY(pointerID));
            int type = -1;
            int idWas = mActivePointerID;
            
        	// Start tracking.
        	if (    mActivePointerID == INVALID_POINTER_ID 
        		 && action == MotionEvent.ACTION_DOWN 
        		 && event.getPointerCount() == 1) 
        	{
        		mActivePointerID = pointerID;
        	}
  	
        	if ( mActivePointerID != pointerID )
        		continue;
        	
	        switch ( action & MotionEvent.ACTION_MASK ) {
	        case MotionEvent.ACTION_DOWN:
	        	type = 0;
	        	break;
	        	
	        case MotionEvent.ACTION_UP:
	        case MotionEvent.ACTION_CANCEL:
	        case MotionEvent.ACTION_POINTER_UP: 
	           	mActivePointerID = INVALID_POINTER_ID;
	           	type = 2;
	        	break;

	        case MotionEvent.ACTION_MOVE:
	        	type = 1;
	        	break;
	        }
	        if ( type >= 0 ) {
	        	lastX = x; lastY = y;
	        	
	        	if ( type != 1 )
	        		Log.v("UFOATTACK", "touch event id=" + idWas + " type=" + type + " x=" + x + " y=" + y );
	        	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, type, x, y, 0 ) );
	        	return true;
	        }
        }        
    	return super.onTouchEvent( event );
    }
    
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
  
    private UFORenderer mRenderer;
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
				break;
			case TYPE_HOTKEY:
				renderer.hotKey( action );
				break;
			case TYPE_ZOOM:
				renderer.zoom( val );
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
    
    public void zoom( float delta ) {
    	nativeZoom( delta );
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
    
    private static native void nativeZoom( float delta );
}
