package com.grinninglizard.UFOAttack;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

import org.apache.http.NameValuePair;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.protocol.HTTP;

import android.content.Context;
import android.util.Log;

public class CrashLogger { //implements Runnable {
//	private String path;
	private Context context;
	
	public CrashLogger( Context context, String path ) {
//		this.path = path;
		this.context = context;
		
		if ( SendCrashLogs() == false ) {	
			// Set a marker that this has been created.
			try {
				FileOutputStream  fos = context.openFileOutput("UFO_Running", Context.MODE_PRIVATE);
				fos.write( 65 );
				fos.close();
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	public void destroy()
	{
		context.deleteFile( "UFO_Running" );
	}
	
	//@Override
	private boolean SendCrashLogs()
	{
		// Look for a crash file.
		//String filepath = path + "/releaseErrorLog.txt";
		boolean logSent = false;
		
		try {
			// This *should* fail - the file is deleted when we close.
			FileInputStream running = context.openFileInput("UFO_Running");
			if ( running != null ) {
				String value = "current game?";
				
		    	FileInputStream fis = context.openFileInput( "currentgame.xml" );
	
		    	if ( fis != null ) {
		    		
		    		StringBuffer strContent = new StringBuffer("");
		    		try {
		    			while( true ) {
		    				int ch = fis.read();
		    				if ( ch < 0 )
		    					break;
		    				strContent.append( (char) ch );
		    			}
					} 
		    		catch (IOException e) {}
		    		
		    		value = strContent.toString();
		    		try {
						fis.close();
					} catch (IOException e) {}
		    	}
				Post( value );
				logSent = true;
				
				try {
					running.close();
				} catch (IOException e) {}
				context.deleteFile( "UFO_Running" );
			}
		} catch (FileNotFoundException e) {
		}
/*
		try {
			BufferedReader input =  new BufferedReader(new FileReader(filepath));
		    try {
		    	String line = ""; //not declared within while loop
		    	while ( ( line = input.readLine()) != null) {
		    		Post( line );
		    	}
		    }
		    finally {
		    	input.close();
		    }
		    
		    File f = new File( filepath );
		    f.delete();
		}
		catch (IOException ex){}
*/		
		return logSent;
	}
	
	private void Post( String data )
	{
		Log.v("UFOATTACK", "Sending error log.");
		// Send it off!
        DefaultHttpClient httpClient = new DefaultHttpClient(); 
        HttpPost httpPost = new HttpPost( "http://grinninglizard.com/collect/server.php" );
        //HttpPost httpPost = new HttpPost( "http://grinninglizard.com/collect/" );
        
        List <NameValuePair> nvps = new ArrayList <NameValuePair>(); 
		nvps.add(new BasicNameValuePair("device", android.os.Build.PRODUCT ));
		nvps.add(new BasicNameValuePair("version", "" + UFOVersion.VERSION ));
		nvps.add(new BasicNameValuePair("stacktrace", data ));

		try {
			httpPost.setEntity(new UrlEncodedFormEntity(nvps, HTTP.UTF_8));
	        httpClient.execute(httpPost);  
		} 
		catch (UnsupportedEncodingException e) {} 
		catch (ClientProtocolException e) {} 
		catch (IOException e) {} 
	}
}
