package ch.fhnw.imvs.audiowalk.actions;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import org.puredata.core.PdBase;

import android.util.Log;
import ch.fhnw.imvs.audiowalk.comm.ActionEventListener;

/**
 * A listener for playing sound files through PD.
 * @author matt
 */
public class PdPlaySoundListener implements ActionEventListener {
	
	static final String OSC_ADDR = "/playSoundfile";
	static final String METHOD_NAME = "handleMessage";	
	static final String PD_RECEIVER = "playSoundfile";

	@Override
	public String getOSCAddress() {
		return OSC_ADDR;
	}

	@Override
	public String getMethodName() {
		return METHOD_NAME;
	}
	
	public void handleMessage(float message) {
		Log.i(LogTag, "Received PD float");
		try {
			PdBase.sendFloat(PD_RECEIVER, message);
		}
		catch (Exception e) {
			Log.e(LogTag, "Cannot pass message to PD.");
		}
	}
	
	public void handleMessage(int message) {
		Log.i(LogTag, "Received PD int");
		try {			
			PdBase.sendFloat(PD_RECEIVER, message);
		}
		catch (Exception e) {
			Log.e(LogTag, "Cannot pass message to PD.");
		}
	}

}
