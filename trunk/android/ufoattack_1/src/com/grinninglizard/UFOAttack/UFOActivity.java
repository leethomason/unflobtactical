/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This is a small port of the "San Angeles Observation" demo
 * program for OpenGL ES 1.x. For more details, see:
 *
 *    http://jet.ro/visuals/san-angeles-observation/
 *
 * This program demonstrates how to use a GLSurfaceView from Java
 * along with native OpenGL calls to perform frame rendering.
 *
 * Touching the screen will start/stop the animation.
 *
 * Note that the demo runs much faster on the emulator than on
 * real devices, this is mainly due to the following facts:
 *
 * - the demo sends bazillions of polygons to OpenGL without
 *   even trying to do culling. Most of them are clearly out
 *   of view.
 *
 * - on a real device, the GPU bus is the real bottleneck
 *   that prevent the demo from getting acceptable performance.
 *
 * - the software OpenGL engine used in the emulator uses
 *   the system bus instead, and its code rocks :-)
 *
 * Fixing the program to send less polygons to the GPU is left
 * as an exercise to the reader. As always, patches welcomed :-)
 */
package com.grinninglizard.UFOAttack;

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
import android.gesture.GestureOverlayView.OnGestureListener;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;

public class UFOActivity extends Activity  {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
       
        mGLView = new DemoGLSurfaceView(this);
        setContentView(mGLView);
        loadUFOAssets();
        
//        mGLView.setOnClickListener( this );
    }

    
  //  public void onClick(View v) {
  //      System.out.println( "onClick ");
  //  }


    @Override
    protected void onPause() {
        super.onPause();
        mGLView.onPause();
        DemoRenderer.nativePause( 1 );
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLView.onResume();
        DemoRenderer.nativePause( 0 );
    }
    
    protected void onDestroy() {
    	DemoRenderer.nativeDone();
    }
    
    private void loadUFOAssets() 
    {
    	// Get the path
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
	        DemoRenderer.nativeResource( apkFilePath, offset, fileSize );
	        afd.close();
	    }
	    catch( IOException e) {
	    	System.out.println( e.toString() );
	    }
	    
	}

    private GLSurfaceView mGLView;

    static {
        System.loadLibrary("ufoattack");
    }
}


class GestureProcessor extends GestureDetector.SimpleOnGestureListener {
	public GestureProcessor() {
		super();
	}
	
	@Override
	public boolean onSingleTapConfirmed(MotionEvent  e) {
		System.out.println( "single tap confirmed " + e.getX() + " " + e.getY() );
		return false;
	}
	
	@Override
	public boolean onSingleTapUp( MotionEvent e ) {
		System.out.println( "single tap confirmed " + e.getX() + " " + e.getY() );
		return false;
	}
}

class DemoGLSurfaceView extends GLSurfaceView implements GestureDetector.OnGestureListener { 
    public DemoGLSurfaceView(Context context) {
        super(context);
        mRenderer = new DemoRenderer();
        setRenderer(mRenderer);
        
        gestureDetector = new GestureDetector( this );
        
        //gestureProcessor = new GestureProcessor();
        //gestureDetector = new GestureDetector( gestureProcessor );
        //setOnTouchListener(gestureDetector);

//        gestureDetector = new GestureDetector(new GestureProcessor() );
//        new OnTouchListener() {
//            public boolean onTouch(View v, MotionEvent event) {
//                if (gestureDetector.onTouchEvent(event)) {
//                    return true;
//                }
//                return false;
//            }
//        };
    }
    
//    public boolean onTouchEvent(MotionEvent  event) {
//		System.out.println( "onTouchEvent" );
//   	return gestureDetector.onTouchEvent( event );
//    }
    
    @Override  
    public boolean onTouchEvent(MotionEvent e) {  
    	return gestureDetector.onTouchEvent(e);  
    }      
    @Override  
    public boolean onDown(MotionEvent e) {  
    	System.out.println( "onDown" ); 
    	return true;  
    }  
          
    @Override  
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {  
    	System.out.println( "onFling" ); 
    	return true;  
    }  
          
    @Override  
    public void onLongPress(MotionEvent e) {  
    	System.out.println( "onLongPress" ); 
    }  
          
    @Override  
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {  
    	System.out.println( "onScroll" ); 
    	return true;  
    }  
          
    @Override  
    public void onShowPress(MotionEvent e) {  
    	System.out.println( "onShowPress" ); 
    }      
          
    @Override  
    public boolean onSingleTapUp(MotionEvent e) {  
    	System.out.println( "onSingleTapUp" ); 
    	return true;  
    }  
    public DemoRenderer mRenderer;
//    private GestureProcessor gestureProcessor;
    private GestureDetector gestureDetector;
}

class DemoRenderer implements GLSurfaceView.Renderer {
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeInit();
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {
        nativeResize(w, h);
    }

    public void onDrawFrame(GL10 gl) {
        nativeRender();
    }

    public static native void nativeResource( String path, long offset, long len );
    private static native void nativeResize(int w, int h);
    private static native void nativeRender();

    private static native void nativeInit();
    public static native void nativePause( int pause );
    public static native void nativeDone();
}
