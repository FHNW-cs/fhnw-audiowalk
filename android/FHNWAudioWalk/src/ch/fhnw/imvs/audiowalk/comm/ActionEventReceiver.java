package ch.fhnw.imvs.audiowalk.comm;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import java.util.ArrayList;
import java.util.List;

import ch.fhnw.imvs.audiowalk.Util;
import oscP5.OscEventListener;
import oscP5.OscMessage;
import oscP5.OscP5;
import oscP5.OscStatus;
import android.content.Context;
import android.util.Log;



/**
 * A listener for action events sent by the tracking server.
 * Events are received through OSC.
 * @author matt
 *
 */
public class ActionEventReceiver implements OscEventListener {
	
	private Context mContext;
	private int mListenPort;
	private OscP5 mOscReceiver;
	private List<ActionEventListener> mEventListeners;
	
	/**
	 * Constructor.
	 * @param context the application context
	 */
	public ActionEventReceiver(Context context) {
		mContext = context;
		mListenPort = -1;
		mOscReceiver = null;
		mEventListeners = new ArrayList<ActionEventListener>();
	}
	
	/**
	 * Sets the IP port to listen on. 
	 * @param port the IP port
	 */
	public void setPort(int port) {
		this.mListenPort = port;
	}
	
	/**
	 * Starts the listener.
	 * @return true if started, false otherwise
	 */
	public boolean start() {
		// already running?
		if (isStarted())
			return true;
		
		// port not set?
		if (mListenPort < 0)
			return false;
		
		// set up socket
		try {
			mOscReceiver = new OscP5(this, mListenPort, OscP5.UDP);
		}
		catch (Exception e) {
			Log.e(LogTag, "Cannot create OSC receiver socket.");
			mOscReceiver = null;
			return false;
		}

		// plug in listeners -> only OSC messages matching address and arguments are processed
		for (ActionEventListener a: mEventListeners) {
			mOscReceiver.plug(a, a.getMethodName(), a.getOSCAddress());
		}
		
		Log.i(LogTag, String.format("Action event receiver started on port %d.", mListenPort));
		return true;
	}
	
	/**
	 * Stops the action event listener.
	 */
	public void stop() {
		if (mOscReceiver != null) {
			try {
				mOscReceiver.stop();
				mOscReceiver.dispose();
			}
			catch (Exception e) {
				Log.e(LogTag, "Action event receiver stop error: " + e.getMessage());
			}
			mOscReceiver = null;
			Log.i(LogTag, "Action event receiver stopped.");
		}
	}
	
	/**
	 * Checks if the action event listener is running.
	 * @return true if running, false if stopped
	 */
	public boolean isStarted() {
		return (mOscReceiver != null);
	}
	
	/**
	 * Adds an action event listener.
	 * Has no effect if the action event receiver is already running.
	 * @param a the listener
	 */
	public void addActionEventListener(ActionEventListener a) {
		mEventListeners.add(a);
	}

	/**
	 * Removes an action event listener.
	 * Has no effect if the action event receiver is already running.
	 * @param a the listener
	 */
	public void removeActionEventListener(ActionEventListener a) {
		mEventListeners.remove(a);
	}
	
	/**
	 * Removes all action event listeners.
	 * Has no effect if the action event receiver is already running.
	 */
	public void removeAllActionEventListeners() {
		mEventListeners.clear();
	}
	
	/**
	 * Default OSC message handler.
	 * @param msg an OSC message
	 */
	@Override
	public void oscEvent(OscMessage msg) {
		if (!msg.isPlugged()) {
			Util.logMessage(mContext, Log.WARN, String.format("Unknown OSC message %s %s", msg.addrPattern(), msg.typetag()), true);
		}
	}

	/**
	 * OSC status handler.
	 * @param status the status
	 */
	@Override
	public void oscStatus(OscStatus status) {
		Util.logMessage(mContext, Log.INFO, String.format("OSC status changed to %d", status), true);
	}
}
