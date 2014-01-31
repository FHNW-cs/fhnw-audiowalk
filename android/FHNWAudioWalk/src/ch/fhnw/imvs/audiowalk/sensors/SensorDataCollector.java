package ch.fhnw.imvs.audiowalk.sensors;

import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

/**
 * A data collector that gathers data from all sensors.
 * Because sensor events appear on the main thread in Android,
 * sensor data is pushed into a queue and processed in a separate thread.
 * @author matt
 *
 */
public class SensorDataCollector implements SensorEventListener {
	
	private Thread mCollectThread;
	private boolean mDoRun;
	private BlockingQueue<SensorEvent> mSensorQueue;
	
	private SensorManager mSensorManager;
	private Sensor mLinAccelSensor;
	private Sensor mGravitySensor;
	private Sensor mGyroSensor;
	private Sensor mOrientationSensor;
	private Sensor mAccelSensor;	
	private RawSensorListener mSensorListener;

	/**
	 * Constructor.
	 * @param sensorManager the Android sensor manager
	 */
	@SuppressWarnings("deprecation")
	public SensorDataCollector(SensorManager sensorManager) {
		mCollectThread = null;
		mDoRun = false;
		mSensorQueue = new LinkedBlockingQueue<SensorEvent>();
		
		mSensorManager = sensorManager;
		mLinAccelSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
		mAccelSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mGyroSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		mOrientationSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION);
	}
	
	/**
	 * Sets the listener for raw sensor data.
	 * @param listener the listener
	 */
	public void setRawSensorListener(RawSensorListener listener) {
		mSensorListener = listener;
	}
	
	/**
	 * Starts collecting sensor data.
	 * @return true if the collector is started, false in case of error
	 */
	public boolean start() {
		
		// already running?
		if (isStarted())
			return true;
		
		// purge pending events
		mSensorQueue.clear();
		
		// start collecting thread
		mCollectThread = new Thread(new Runnable() {

			@Override
			public void run() {
				mDoRun = true;
				
				while (mDoRun) {					
					SensorEvent event = null;
					
					try {
						event = mSensorQueue.poll(200, TimeUnit.MILLISECONDS);
					}
					catch (InterruptedException e) {
						Log.e(LogTag, "SensorQueue interrupted.");
					}

					if (event != null) {
						// handle raw sensor data
						if (mSensorListener != null) {
							mSensorListener.handleSensorData(event.sensor.getType(), event.values);
						}
					}
				}				
			}
			
		});		
		mCollectThread.start();
		
		// register sensor listeners
		
		mSensorManager.registerListener(this, mGravitySensor, 40000);
		mSensorManager.registerListener(this, mGyroSensor, 40000);
		mSensorManager.registerListener(this, mOrientationSensor, 250000);
		mSensorManager.registerListener(this, mLinAccelSensor, 40000);
		mSensorManager.registerListener(this, mAccelSensor, 40000);
		
		return true;
	}
	
	/**
	 * Stops collecting sensor data.
	 */
	public void stop() {//
		if (mCollectThread != null) {
			
			// unregister all sensor listeners
			mSensorManager.unregisterListener(this);
			
			// stop collecting
			mDoRun = false;
			
			try {
				mCollectThread.join(1000);
			}
			catch (InterruptedException e) {
				Log.e(LogTag, "CollectThread interrupted.");
			}
			mCollectThread = null;
			
			Log.i(LogTag, "Sensor data collector stopped.");
		}
	}
	
	/**
	 * Checks if the sensor data collector is running.
	 * @return true if running, false if stopped
	 */
	public boolean isStarted() {
		return ((mCollectThread != null) && mCollectThread.isAlive());
	}
	
	/**
	 * Called when a sensor accuracy is changed.
	 * @param sensor the sensor
	 * @param accuracy the new accuracy
	 */
	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		// TODO do something if necessary		
	}

	/**
	 * Called when new sensor data is received.
	 * Pushes the sensor data into a queue which is processed in a separate thread.
	 * @param event the sensor event
	 */
	@Override
	public void onSensorChanged(SensorEvent event) {
		try {
			mSensorQueue.put(event);	
		}
		catch (InterruptedException e) {
			Log.e(LogTag, "Cannot enqueue sensor event.");
		}	
	}

}
