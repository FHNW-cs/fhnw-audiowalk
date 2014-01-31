package ch.fhnw.imvs.audiowalk.events;

import oscP5.OscMessage;


/**
 * A tracking event that provides position data.
 * @author matt
 *
 */
public class PositionEvent implements TrackingEvent {
	
	private static final String OSC_ADDRESS = "/position";
	
	private int deviceNr;
	private int locality;
	private float x;
	private float y;

	/**
	 * Constructor.
	 * @param deviceNr the device number
	 * @param locality the locality ID
	 * @param x the X coordinate
	 * @param y the Y coordinate
	 */
	public PositionEvent(int deviceNr, int locality, float x, float y) {
		this.deviceNr = deviceNr;
		this.locality = locality;
		this.x = x;
		this.y = y;
	}
	
	/**
	 * Converts the event to an OSC message.
	 * @return an OSC message
	 */
	@Override
	public OscMessage getOSCMessage() {
		OscMessage msg = new OscMessage(OSC_ADDRESS);
		msg.add(deviceNr);
		msg.add(locality);
		msg.add(x);
		msg.add(y);
		return msg;
	}

}
