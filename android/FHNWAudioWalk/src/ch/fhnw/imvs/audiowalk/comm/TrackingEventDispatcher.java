package ch.fhnw.imvs.audiowalk.comm;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import oscP5.OscP5;
import netP5.NetAddress;
import android.util.Log;
import ch.fhnw.imvs.audiowalk.events.TrackingEvent;


/**
 * A dispatcher for tracking events.
 * Takes events created by the app's tracking modules and sends them to
 * the tracking server using OSC.
 * @author matt
 *
 */
public class TrackingEventDispatcher {
	
	public static final int REPEAT_OSC_EVENTS = 1;
	
	private NetAddress serverAddress;	
	private BlockingQueue<TrackingEvent> eventQueue;	
	private Thread dispatchThread;
	private boolean doRun;
	private int repeatEvents;
	
	/**
	 * Constructor.
	 */
	public TrackingEventDispatcher() {
		serverAddress = null;
		eventQueue = new LinkedBlockingQueue<TrackingEvent>();
		dispatchThread = null;
		doRun = false;
		repeatEvents = REPEAT_OSC_EVENTS;
	}
	
	/**
	 * Sets the tracking server address. 
	 * @param addr the IP address
	 * @param port the IP port
	 */
	public void setTrackingServer(String addr, int port) {//
		serverAddress = new NetAddress(addr, port);
		Log.i(LogTag, "setTrackingServer " + addr + ":" + port);
		
	}
	
	/**
	 * Sets the tracking server address. 
	 * @param addr the address and port
	 */
	public void setTrackingServer(NetAddress addr) {
		serverAddress = addr;
		Log.i(LogTag, "setTrackingServer addr " + addr.address());
	}
	
	/**
	 * Starts the dispatcher thread.
	 * @return true if started, false otherwise
	 */
	public boolean start() {
		// already running?
		if (isStarted())
			return true;
		
		// IP address or port not set?
		if (serverAddress == null)
		{
			Log.i(LogTag, "Server not set");
			return false;
		}
		
		// purge any pending events
		eventQueue.clear();
		
		// start dispatch thread
		dispatchThread = new Thread(new Runnable() {

			@Override
			public void run() {
				doRun = true;
				
				while (doRun) {
					TrackingEvent event = null;
					
					try {
						event = eventQueue.poll(200, TimeUnit.MILLISECONDS);
					}
					catch (InterruptedException e) {
						Log.e(LogTag, "EventQueue interrupted");
					}

					if ((event != null) && (serverAddress != null)) {
						try {
							// send event to the tracking server
							// repeat to ensure event arrives
							for (int i = 0; i < repeatEvents; i++)
								OscP5.flush(event.getOSCMessage(), serverAddress);
						}
						catch (Exception e) {
							Log.e(LogTag, "Cannot send tracking event.");
						}
					}
				}
			}
			
		});		
		dispatchThread.start();
		Log.i(LogTag, "Tracking event dispatcher started.");
		return true;
	}
	
	/**
	 * Stops the dispatcher thread.
	 */
	public void stop() {
		if (dispatchThread != null) {
			doRun = false;

			try {
				dispatchThread.join(1000);
			}
			catch (InterruptedException e) {
			}
			dispatchThread = null;
			Log.i(LogTag, "Tracking event dispatcher stopped.");
		}
	}
	
	/**
	 * Checks if the event dispatcher is running.
	 * @return true if running, false if stopped
	 */
	public boolean isStarted() {
		return ((dispatchThread != null) && dispatchThread.isAlive());
	}
	
	/**
	 * Puts an event into the dispatcher queue.
	 * @param event the tracking event
	 */
	public void dispatchEvent(TrackingEvent event) {
		
		try {
			eventQueue.put(event);
		}
		catch (InterruptedException e) {
			Log.e(LogTag, "Cannot enqueue tracking event.");
		}
	
	}
}
