package ch.fhnw.imvs.audiowalk.actions;

import android.content.Context;
import android.util.Log;
import ch.fhnw.imvs.audiowalk.RuntimeConfiguration;
import ch.fhnw.imvs.audiowalk.Util;
import ch.fhnw.imvs.audiowalk.comm.ActionEventListener;


/**
 * A listener for incoming position events.
 * Used for external position tracking.
 * @author matt
 */

public class PositionEventListener implements ActionEventListener {
	
	static final String OSC_ADDR = "/position";
	static final String METHOD_NAME = "handlePositionEvent";
	
	static final long STEPCOUNTER_DELAY = 500;
	
	private RuntimeConfiguration mConfig;
	private Context mContext;

	/**
	 * Constructor.
	 * @param context the application context
	 * @param config the runtime configuration
	 */
	public PositionEventListener(Context context, RuntimeConfiguration config) {
		mContext = context;
		mConfig = config;
	}
	
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
	public void handlePositionEvent(int deviceNr, int locality, float x, float y) {
		if (deviceNr == mConfig.deviceNr) {
			mConfig.currentPosition.x = x;
			mConfig.currentPosition.y = y;
			mConfig.checkPositionLimit();
			Util.logMessage(mContext, Log.INFO, String.format("New position: %d (%.1f, %.1f)", locality, x, y), mConfig.displayPosition);
		}
	}
}
