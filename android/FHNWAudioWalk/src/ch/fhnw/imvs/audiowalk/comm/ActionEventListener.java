package ch.fhnw.imvs.audiowalk.comm;

/**
 * An interface for an action event listener.
 * @author matt
 *
 */
public interface ActionEventListener {

	/**
	 * Gets the OSC address.
	 * @return a string
	 */
	String getOSCAddress();
	
	/**
	 * Gets the handler method name.
	 * @return a string
	 */
	String getMethodName();
}
