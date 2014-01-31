package ch.fhnw.imvs.audiowalk.actions;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import org.puredata.core.PdBase;
import android.util.Log;
import ch.fhnw.imvs.audiowalk.comm.ActionEventListener;

/**
 * A listener for general PD actions.
 * @author matt
 */
public class PdActionListener implements ActionEventListener {
	
	static final String OSC_ADDR = "/pdaction";
	static final String METHOD_NAME = "handleGeneralAction";
	static final String PD_RECEIVER = "pdaction";
	
	static final Object[] DUMMY = new Object[] { };

	@Override
	public String getOSCAddress() {
		return OSC_ADDR;
	}

	@Override
	public String getMethodName() {
		return METHOD_NAME;
	}
	
	/**
	 * Handles a PD event.
	 * @param message the message to be passed to PD
	 */
	public void handleGeneralAction(String message) {
		Log.i(LogTag, "Received PD action " + message);
		try {
			PdBase.sendMessage(PD_RECEIVER, message, DUMMY);
		}		
		catch (Exception e) {
			Log.e(LogTag, "Cannot pass message to PD.");
		}
	}

}
