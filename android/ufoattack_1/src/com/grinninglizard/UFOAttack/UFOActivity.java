
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
import android.view.KeyEvent;
import android.view.MotionEvent;

public class UFOActivity extends Activity  {
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // On the main thread, before we fire off the render thread:
        loadUFOAssets();
        setWritePaths();
       
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
    		UFORenderer.nativeSavePath( file.getAbsolutePath() );
    	}
    }

    
    private DemoGLSurfaceView mGLView;

    static {
        System.loadLibrary("ufoattack");
    }
}


class DemoGLSurfaceView extends GLSurfaceView  { 

	public DemoGLSurfaceView(Context context) {
        super(context);
        mRenderer = new UFORenderer();
        setRenderer(mRenderer);
        
        requestFocusFromTouch();
        setFocusableInTouchMode( true );
        //setClickable( true );
    }
	
	// @Override
	public void onPause() {
		super.onPause();
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_PAUSE, 0, 0, 0 ) );
	}
	
	// @Override
	public void onResume() {
		super.onResume();
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_RESUME, 0, 0, 0 ) );
	}
	
	public void destroy() {
		queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_DESTROY, 0, 0, 0 ) );
	}

	
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        
        int x = (int)event.getX();
        int y = (int)(getHeight() - event.getY());
        int type = -1;
        
        if (action == MotionEvent.ACTION_DOWN ) {
            //Log.v( "UFOJAVA", "Action down at " + event.getX() + "," + event.getY() );
        	type = 0;
        }
        else if ( action == MotionEvent.ACTION_UP ) {
        	type = 2;
        }
        else if ( action == MotionEvent.ACTION_MOVE ) {
        	type = 1;
        }
        if ( type >= 0 ) {
        	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_TAP, type, x, y ) );
        	return true;
        }
        else {
        	return super.onTouchEvent( event );
        }
    }
    
    
   //  @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    	int sendKey = 0;
    	int GAME_HK_NEXT_UNIT	=		0x0001;
    	int GAME_HK_PREV_UNIT	= 		0x0002;
    	int GAME_HK_ROTATE_CCW	=		0x0004;
    	int GAME_HK_ROTATE_CW	=		0x0008;

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
        	queueEvent( new RendererEvent( mRenderer, RendererEvent.TYPE_HOTKEY, sendKey, 0, 0 ) );
            return true;
        }
    	else {
    		return super.onKeyDown(keyCode, event);
    	}
    }
  
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
	
	private UFORenderer renderer;
	private int type;
	private int action;
	private int x;
	private int y;
	
	RendererEvent( UFORenderer renderer, int type, int action, int x, int y ) 
	{
		this.renderer = renderer;
		this.type = type;
		this.action = action;
		this.x = x;
		this.y = y;
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

    public static native void nativeResource( String path, long offset, long len );
    public static native void nativeSavePath( String path );
    
    private static native void nativeResize(int w, int h);
    private static native void nativeRender();

    private static native void nativeInit();
    private static native void nativePause( int pause );
    private static native void nativeDone();
    
    private static native void nativeTap( int action, float x, float y );
    private static native void nativeHotKey( int key );
}
