package ch.fhnw.imvs.audiowalk.actions;


import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import java.io.IOException;
import org.puredata.core.PdBase;

import ch.fhnw.imvs.audiowalk.PureDataUtils;
import ch.fhnw.imvs.audiowalk.RuntimeConfiguration;
import ch.fhnw.imvs.audiowalk.comm.ActionEventListener;
import android.util.Log;

/**
 * A listener for loading new patches in PD.
 * only used for debugging purposes
 * @author matt
 */
public class PdLoadPatchListener implements ActionEventListener {
	
	static final int LOAD_DELAY = 1000;
	
	static final String PD_RECEIVER_FADEOUT = "fadeOut";
	static final String OSC_ADDR = "/loadPatch";
	static final String METHOD_NAME = "handleMessage";
	
	RuntimeConfiguration mConfig;
	
	public PdLoadPatchListener(RuntimeConfiguration config) {
		mConfig = config;
	}

	@Override
	public String getOSCAddress() {
		return OSC_ADDR;
	}

	@Override
	public String getMethodName() {//
		return METHOD_NAME;
	}
	
	public void handleMessage(String message) {
		final String messageCopy = message;
		Log.i(LogTag, "Received load new patcher message " + messageCopy);//
		PdBase.sendBang(PD_RECEIVER_FADEOUT);
		mConfig.handler.postDelayed(new Runnable() {
		  @Override
		  public void run() {
			  Log.i(LogTag, "Enter Thread");
			  try {
				PureDataUtils.loadPdPatch(mConfig, messageCopy);
				Log.i(LogTag, "Loaded Patch" + messageCopy);
				
			} catch (IOException e) {
				Log.e(LogTag, "Could not load patch" + messageCopy);
			}
		  }
		}, LOAD_DELAY);		
	}
}
