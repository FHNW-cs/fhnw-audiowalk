package ch.fhnw.imvs.audiowalk.events;

import oscP5.OscMessage;


/**
 * An interface for tracking events.
 * @author matt
 *
 */
public interface TrackingEvent {

	/**
	 * Gets an OSC message to be sent to the tracking server.
	 * @return an OSC message
	 */
	OscMessage getOSCMessage();
}
