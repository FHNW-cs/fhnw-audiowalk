package ch.fhnw.imvs.audiowalk.sensors;

/**
 * A listener interface for raw sensor data.
 * @author matt
 *
 */
public interface RawSensorListener {

	/**
	 * Handles raw sensor data.
	 * @param type the sensor type according to android.hardware.Sensor
	 * @param values the sensor values
	 */
	void handleSensorData(int type, float[] values);
}

