package ch.fhnw.imvs.audiowalk;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;
import android.content.Context;
import android.content.Intent;
import android.util.Log;//


/**
 * Utility functions.
 * @author matt
 *
 */
public class Util {

	/**
	 * Checks if a string contains a valid IP address (IPv4).
	 * @param str a string
	 * @return true if a IP address, false otherwise
	 */
	public static boolean isValidIPAddress(String str) {
		
		if (str == null) {
			return false;
		}
		String[] parts = str.trim().split("\\.");
		if (parts.length == 4) {
			for (int i = 0; i < 4; i++) {
				try {
					int octet = Integer.parseInt(parts[i]);
					if ((octet < 0) || (octet > 255)) {
						return false;
					}
				}
				catch (Exception e) {
					return false;
				}
			}
		}
		else {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Checks if a string contains a valid IP port (0..65535).
	 * @param str a string
	 * @return true if a port, false otherwise
	 */
	public static boolean isValidPort(String str) {
		
		if (str == null) {
			return false;
		}
		int port = -1;
		try {
			port = Integer.parseInt(str.trim());
			if ((port < 0) || (port > 65535)) {
				return false;
			}
		}
		catch (Exception e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Checks if a string contains a valid decimal number.
	 * @param str a string
	 * @return true if decimal number, false otherwise
	 */
	public static boolean isValidDecimal(String str) {
		
		if (str == null) {
			return false;
		}
		try {
			Float.parseFloat(str.trim());
			return true;
		}
		catch (Exception e) {
			return false;
		}
	}
	
	/**
	 * Logs a message to the debug console and optionally sends it to a display using broadcast.
	 * @param context the application context
	 * @param logLevel the log level
	 * @param message the message to log
	 * @param display true if to be displayed in activity
	 */
	

	public static void logMessage(Context context, int logLevel, String message, boolean display) {
		Log.println(logLevel, LogTag, message);
		if (display) {
			// send intent to display in activity
			Intent intent = new Intent(Globals.LOG_MESSAGE_ACTION);
			intent.putExtra("message", message);
			context.sendBroadcast(intent);
		}
	}
}

