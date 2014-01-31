package ch.fhnw.imvs.audiowalk.events;

import oscP5.OscMessage;

/**
 * A device registration event.
 * The device registers on MaxMSP.
 * @author matt
 *
 */
public class RegistrationEvent implements TrackingEvent {
	
	private static final String OSC_ADDRESS = "/registermobile";
	
	private int deviceNr;
	private String ip;
	private int port;
	private int localityId;

	/**
	 * Constructor.
	 * @param deviceNr the device number
	 * @param ip the IP address of the device
	 * @param port the UDP port OSC listens on//
	 * @param locality the locality ID where the device is registered
	 */
	public RegistrationEvent(int deviceNr, String ip, int port, int locality) {
		this.deviceNr = deviceNr;
		this.ip = ip;
		this.port = port;
		this.localityId = locality;
	}
	
	/**
	 * Converts the event to an OSC message.
	 * @return an OSC message
	 */
	@Override
	public OscMessage getOSCMessage() {
		OscMessage msg = new OscMessage(OSC_ADDRESS);
		msg.add(deviceNr);
		msg.add(ip);
		msg.add(port);
		msg.add(localityId);
		return msg;
	}
}
