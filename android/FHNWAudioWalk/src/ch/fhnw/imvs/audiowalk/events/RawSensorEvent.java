package ch.fhnw.imvs.audiowalk.events;

import oscP5.OscMessage;



/**
 * A tracking event that provides raw sensor data.
 * @author matt
 *
 */
public class RawSensorEvent implements TrackingEvent {
	
	private static final String OSC_ADDRESS = "/sensors/raw";
	
	private int deviceNr;
	private int type;
	private float[] values;

	/**
	 * Constructor.
	 * @param type the sensor type (see SensorType)
	 * @param values the sensor values (an array of float)
	 */
	public RawSensorEvent(int deviceNr, int type, float[] values) {
		this.deviceNr = deviceNr;
		this.type = type;
		this.values = values;
	}
	
	/**
	 * Converts the event to an OSC message.//
	 * @return an OSC message
	 */
	@Override
	public OscMessage getOSCMessage() {
		OscMessage msg = new OscMessage(OSC_ADDRESS);
		msg.add(deviceNr);
		msg.add(type);		
		for (int i = 0; i < values.length; i++) {
			msg.add(values[i]);
		}		
		return msg;
	}
}
