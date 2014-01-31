package ch.fhnw.imvs.audiowalk.service;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.Math;
import netP5.NetAddress;
import org.json.JSONArray;
import org.json.JSONObject;
import org.puredata.android.io.AudioParameters;
import org.puredata.android.service.PdService;
import org.puredata.core.PdBase;

import ch.fhnw.imvs.audiowalk.Globals;
import ch.fhnw.imvs.audiowalk.R;
import ch.fhnw.imvs.audiowalk.RuntimeConfiguration;
import ch.fhnw.imvs.audiowalk.actions.*;
import ch.fhnw.imvs.audiowalk.comm.ActionEventListener;
import ch.fhnw.imvs.audiowalk.comm.ActionEventReceiver;
import ch.fhnw.imvs.audiowalk.comm.TrackingEventDispatcher;
import ch.fhnw.imvs.audiowalk.data.*;
import ch.fhnw.imvs.audiowalk.events.*;
import ch.fhnw.imvs.audiowalk.sensors.RawSensorListener;
import ch.fhnw.imvs.audiowalk.sensors.SensorDataCollector;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.hardware.SensorManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.Binder;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceManager;
import android.text.format.Formatter;
import android.util.Log;
import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

/**
 * The background service for the audio walk app.
 * Almost all the functionality is integrated into this service and keeps running in the background,
 * even if the UI activities are hidden, and even if the screen is off.
 * @author matt
 *
 */
public class AudioWalkService extends Service implements RawSensorListener {
	
	private static final String PD_RAWSENSORS_RECEIVER = "rawsensors";
	private static final String LOCALITY_FILE = "localities.json";
	private static final String CONFIG_FILE = "config.json";
	
	static final float DEG2RAD = (float)(Math.PI / 180);
	
	private final IBinder binder = new AudioWalkServiceBinder();//
	
	private WakeLock wakeLock;
	private WifiLock wifiLock;
	
	private RuntimeConfiguration mConfig;
	
	private ActionEventReceiver actionEventReceiver;
	private TrackingEventDispatcher trackingEventDispatcher;
	private SensorDataCollector sensorCollector;
	
	private ActionEventListener pdPlaySoundListener = null;
	private ActionEventListener pdActionListener = null;
	private ActionEventListener pdLoadPatchListener = null;
	private ActionEventListener positionListener = null;
	
	private ConnectivityManager connMgr;
	private WifiManager wifiMgr;
	private String lastSsid = null;
	
	private boolean prefsChanged = false;
	 
	private BroadcastReceiver connectivityReceiver = new BroadcastReceiver() {//
		@Override
  		public void onReceive(Context context, Intent intent) {
			onConnectivityChanged();	
		}
	};

	
	/**
	 * Connection to PD service.
	 */
	private final ServiceConnection pdConn = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConfig.pdService = ((PdService.PdBinder) service).getService();
			try {
				initPd();
			}
			catch (IOException e) {
				logMessage(Log.ERROR, e.toString(), false);
			}
			if (mConfig.pdEnabled && (mConfig.pdService != null)) {
				mConfig.pdService.startAudio();
				logMessage(Log.INFO, "PD processing started.", true);
			}
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConfig.pdService = null;
		}

	};
	
	/**
	 * Called when the service is created.
	 */
	@Override
	public void onCreate() {
		super.onCreate();
		
		mConfig = new RuntimeConfiguration();
		
		connMgr = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
		wifiMgr = (WifiManager) getSystemService(Context.WIFI_SERVICE);
		
		// load in-app preferences
		if (!loadPreferences()) {
			logMessage(Log.ERROR, "Preferences have errors!", true);
		}
		
		
		// load config (JSON)
		try {
			loadConfiguration();
			logMessage(Log.INFO, "Configuration loaded.", true);
		}
		catch (Exception e) {
			logMessage(Log.ERROR, "Error loading configuration: " + e.getMessage(), true);
		}

		mConfig.localitiesById.clear();
		mConfig.localitiesBySsid.clear();
		mConfig.currentLocality = null;
		mConfig.wifiEnabled = true;
		
		try {
			loadLocalities();
			logMessage(Log.INFO, "Localities loaded.", true);
		}
		catch (Exception e) {
			logMessage(Log.ERROR, "Error loading localities: " + e.getMessage(), true);
		}
		
		positionListener = new PositionEventListener(this.getApplicationContext(), mConfig);
		
		trackingEventDispatcher = new TrackingEventDispatcher();
		actionEventReceiver = new ActionEventReceiver(this.getApplicationContext());
		
		// receive external absolute position events
		actionEventReceiver.addActionEventListener(positionListener);		
		
		pdPlaySoundListener = new PdPlaySoundListener();
		pdActionListener = new PdActionListener();
		pdLoadPatchListener = new PdLoadPatchListener(mConfig);
		
		SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
		sensorCollector = new SensorDataCollector(sm);
		
		if (mConfig.oscLocalPort >= 0) {
			actionEventReceiver.setPort(mConfig.oscLocalPort);//
		}
		
		if (mConfig.pdEnabled) {
			actionEventReceiver.addActionEventListener(pdPlaySoundListener);
			actionEventReceiver.addActionEventListener(pdActionListener);
			actionEventReceiver.addActionEventListener(pdLoadPatchListener);
		}
		
		/*
		 * could get dynamically added and removed like the other
		 * listeners
		 */
		
		if (mConfig.sensorsEnabled) {
			sensorCollector.setRawSensorListener(this);
		}
		
		// bind PD service
		if (!bindService(new Intent(this.getApplicationContext(), PdService.class), pdConn, BIND_AUTO_CREATE)) {
			logMessage(Log.ERROR, "Cannot bind to PD service.", true);
		}
		
		// reconnect if device number changed.
		if (prefsChanged) {
			prefsChanged = false;
			lastSsid = null;
			onConnectivityChanged();
		}
		
		logMessage(Log.INFO, "Audio walk service created.", false);
	}
	
	/**
	 * Called when the service is destroyed.
	 */
	@Override
	public void onDestroy() {
		// clean up
		stopTasks();
		
		if ((wifiLock != null) && wifiLock.isHeld()) {
			wifiLock.release();
		}
		
		if ((wakeLock != null) && wakeLock.isHeld()) {
			wakeLock.release();
		}
		
		if (mConfig.pdPatchHandle >= 0)
			PdBase.closePatch(mConfig.pdPatchHandle);
		PdBase.release();
		unbindService(pdConn);
		
		logMessage(Log.INFO, "Audio walk service destroyed.", false);
		super.onDestroy();
	}
	
	/**
	 * Called when the service is started.
	 * @param intent the given intent
	 * @param flags additional flags to the intent
	 * @param startId a unique integer that represents the start request
	 */
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		logMessage(Log.INFO, "Audio walk service started.", true);
		return Service.START_STICKY;
	}

	/**
	 * Called when the service is bound by an activity.
	 * @param intent the bind intent
	 */
	@Override
	public IBinder onBind(Intent intent) {
		return binder;
	}
	
	/**
	 * Starts all background tasks.
	 */
	public void startTasks() {
		
		if (wakeLock == null) {
			// keep all tasks, such as tracking or networking, awake, even when display is turned off
			PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
			wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "AudioWalkService");
			wakeLock.acquire();			
		}
		
		enableWifi(mConfig.wifiEnabled);

		if (mConfig.oscEnabled) {
			if (!actionEventReceiver.isStarted()) {
				if (!actionEventReceiver.start())
					logMessage(Log.ERROR, "Cannot start action event receiver.", true);
				// force register?
				lastSsid = null;
				onConnectivityChanged();
			}
			
			if (!trackingEventDispatcher.isStarted()) {
				if (!trackingEventDispatcher.start())
					logMessage(Log.ERROR, "Cannot start tracking event dispatcher.", true);
			}
		}
		
		if (mConfig.sensorsEnabled) {
			if (!sensorCollector.isStarted()) {
				if (!sensorCollector.start())
					logMessage(Log.ERROR, "Cannot start sensor data collector.", true);
			}
		}
		
		if (mConfig.pdEnabled) {
			if (mConfig.pdService != null) {
				mConfig.pdService.startAudio();
			}
			else {
				logMessage(Log.ERROR, "Cannot start PD audio because service is not bound.", false);
			}
		}
		
		registerReceiver(connectivityReceiver, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
		
		logMessage(Log.INFO, "Background tasks started.", false);
	}
		
	
	/**
	 * Stops all background tasks.
	 */
	public void stopTasks() {
		
		try {
			unregisterReceiver(connectivityReceiver);
		}
		catch (IllegalArgumentException e) {
		}
		sensorCollector.stop();
		actionEventReceiver.stop();
		trackingEventDispatcher.stop();
		
		if (mConfig.pdService != null) {
			mConfig.pdService.stopAudio();
		}
		else {
			Log.e(LogTag, "Cannot stop PD audio because service is not bound.");
		}
		
		if ((wifiLock != null) && wifiLock.isHeld()) {
			wifiLock.release();
			wifiLock = null;
		}
		
		if ((wakeLock != null) && wakeLock.isHeld()) {
			wakeLock.release();
			wakeLock = null;
		}
		
		logMessage(Log.INFO, "Background tasks stopped.", false);
	}
	

	
	/**
	 * Initializes the PD audio library.
	 * @throws IOException
	 */
	private void initPd() throws IOException {
		
		int sampleRate = AudioParameters.suggestSampleRate();
		mConfig.pdService.initAudio(sampleRate, 0, 2, 10.0f);
		// the patch is loaded now in onConnectivityChanged()
		// the patchName must be added to the json file for every room
	}
	
	/**
	 * Load the in-app preferences.
	 * @return true if no errors, false otherwise
	 */
	public boolean loadPreferences() {
		boolean errors = false;
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this.getApplicationContext());
		String deviceNrStr = prefs.getString(getString(R.string.pref_device_nr), null);
		mConfig.deviceNr = -1;
		if (deviceNrStr != null) {
			try {
				int deviceNr = Integer.parseInt(deviceNrStr);
				if ((deviceNr != mConfig.deviceNr) && (deviceNr >= 0)) {
					mConfig.deviceNr = deviceNr;
					logMessage(Log.INFO, "Changed device number to " + mConfig.deviceNr, true);
					prefsChanged = true;
				}
			}
			catch (Exception e) {
			}
		}
		if (mConfig.deviceNr <= 0) {
			logMessage(Log.ERROR, "Invalid device number!", true);
			errors = true;
		}
		
		return !errors;
	}
	
	/**
	 * Loads configuration from file.
	 * @throws Exception
	 */
	private void loadConfiguration() throws Exception {
		
		String path = mConfig.baseDataDir + CONFIG_FILE;
		
		FileInputStream instream = new FileInputStream(path);
		if (instream != null) {
			InputStreamReader inputreader = new InputStreamReader(instream);
            BufferedReader buffreader = new BufferedReader(inputreader);
            StringBuilder sb = new StringBuilder();
            String line = null;
            while ((line = buffreader.readLine()) != null) {
            	sb.append(line);
            }            
            buffreader.close();
            
            JSONObject jsonData = new JSONObject(sb.toString());
            
            // get properties.
            // defaults are used if they are not specified.
            mConfig.oscEnabled = jsonData.optBoolean("oscEnabled", false);
            mConfig.oscLocalPort = jsonData.optInt("oscLocalPort", 9001);
            mConfig.pdEnabled = jsonData.optBoolean("pdEnabled", false);
            mConfig.sensorsEnabled = jsonData.optBoolean("sensorsEnabled", false);
            mConfig.rawSensorDataToOsc = jsonData.optBoolean("rawSensorsToOsc", false);
            mConfig.rawSensorDataToPd = jsonData.optBoolean("rawSensorsToPd", false);
            mConfig.displayPosition = jsonData.optBoolean("displayPosition", false);
		}
	}
	
	/**
	 * Load the locality definitions.
	 * @throws Exception
	 */
	private void loadLocalities() throws Exception {
		
		String path = mConfig.baseDataDir + LOCALITY_FILE;
		
		FileInputStream instream = new FileInputStream(path);
		if (instream != null) {
			InputStreamReader inputreader = new InputStreamReader(instream);
            BufferedReader buffreader = new BufferedReader(inputreader);
            StringBuilder sb = new StringBuilder();
            String line = null;
            while ((line = buffreader.readLine()) != null) {
            	sb.append(line);
            }
            buffreader.close();
            
            JSONObject jsonData = new JSONObject(sb.toString());
            
            // get localities
            JSONArray localityArray = jsonData.getJSONArray("localities");
            for (int i = 0; i < localityArray.length(); i++) {
            	JSONObject jl = localityArray.getJSONObject(i);
            	Locality l = new Locality();
            	l.id = jl.getInt("id");//
            	l.name = jl.getString("name");
            	l.ssid = jl.getString("ssid");
            	l.pdPatcherName = jl.getString("pdPatcherName");
            	
            	JSONObject maxmsp = jl.optJSONObject("maxmsp");
            	if (maxmsp != null) {
            		l.maxmsp = new NetAddress(maxmsp.getString("address"), maxmsp.getInt("port"));
            	}  
            	else {
            		Log.e(LogTag, "no MaxMSP address for locality " + l.id);//
            	}
            		
            	JSONObject grid = jl.getJSONObject("grid");
            	l.minX = (float)grid.getDouble("xmin");
            	l.maxX = (float)grid.getDouble("xmax");
            	l.minY = (float)grid.getDouble("ymin");
            	l.maxY = (float)grid.getDouble("ymax");
            	
            	mConfig.localitiesById.put(l.id, l);
            	mConfig.localitiesBySsid.put(l.ssid, l);
            }

		}
	}
	
	/**
	 * Logs a message to the debug console and optionally stores it in a message list
	 * that can be displayed in an activity.
	 * @param logLevel the log level
	 * @param message the message to log
	 * @param display true if to be displayed in activity
	 */
	private void logMessage(int logLevel, String message, boolean display) {
		Log.println(logLevel, LogTag, message);
		if (display) {
			// send intent to display in activity
			Intent intent = new Intent(Globals.LOG_MESSAGE_ACTION);
			intent.putExtra("message", message);
			sendBroadcast(intent);
		}
	}

	
	public class AudioWalkServiceBinder extends Binder {
		
		/**
		 * Gets the current service instance.
		 * @return a service instance
		 */
		public AudioWalkService getService() {
			return AudioWalkService.this;
		}
	}

	/**
	 * Handles raw sensor data.
	 * @param type the sensor type
	 * @param values the sensor values
	 */
	@Override
	public void handleSensorData(int type, float[] values) {//
		
		if (mConfig.oscEnabled && mConfig.rawSensorDataToOsc) {
			// send raw sensor data through OSC
			if (trackingEventDispatcher.isStarted()) {
				trackingEventDispatcher.dispatchEvent(new RawSensorEvent(mConfig.deviceNr, type, values));
			}
		}
		
		if (mConfig.pdEnabled && mConfig.rawSensorDataToPd) {
			try {
				Object[] params = new Object[values.length + 1];
				params[0] = type;
				for (int i = 0; i < values.length; i++) {
					params[i+1] = values[i];
				}
				PdBase.sendList(PD_RAWSENSORS_RECEIVER, params);
			}
			catch (Exception e) {
			}
		}
	}

	
	/**
	 * Handles connectivity changes, e.g. when the WiFi network is changed.
	 */
	private void onConnectivityChanged() {
		
		NetworkInfo info = connMgr.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		if ((info != null) && (info.isConnected())) {
			Log.i(LogTag, "WiFi connected");
			WifiInfo wifi = wifiMgr.getConnectionInfo();
			if (wifi != null) {
				// fix SSID being returned with quotation marks
				String ssid = wifi.getSSID().replaceAll("\"", "");
				if (ssid.equals(lastSsid)) {
					Log.i(LogTag, "SSID unchanged");
				}
				else {
					Log.i(LogTag, "SSID changed");
					lastSsid = ssid;
					
					mConfig.currentLocality = mConfig.localitiesBySsid.get(ssid);
					if (mConfig.currentLocality != null) {
						logMessage(Log.INFO, "Changed locality to " + mConfig.currentLocality.name, true);
						trackingEventDispatcher.setTrackingServer(mConfig.currentLocality.maxmsp); //removed loadPatch; is done manually on a gate event occurs
						
						if (mConfig.oscEnabled) {
							if (trackingEventDispatcher.start()) {
								//// register at server
								@SuppressWarnings("deprecation")
								RegistrationEvent event = new RegistrationEvent(mConfig.deviceNr, Formatter.formatIpAddress(wifi.getIpAddress()), mConfig.oscLocalPort, mConfig.currentLocality.id);
								Log.i(LogTag, "Registering device");
								trackingEventDispatcher.dispatchEvent(event);
							}
							else {
								Log.i(LogTag, "Could not register device");
							}

						}
					}
					else {
						logMessage(Log.WARN, "No locality for SSID " + ssid, true);
						if (mConfig.oscEnabled) {
							trackingEventDispatcher.stop();
						}
					}
				}
			}
		}
		else {
			Log.i(LogTag, "WiFi disconnected");
			if (mConfig.oscEnabled) {
				trackingEventDispatcher.stop();
			}
			lastSsid = null;
		}
	}

	
	/**
	 * Enable or disable WiFi.
	 * @param enable
	 */
	private void enableWifi(boolean enable) {
		if (enable) {
			wifiMgr.setWifiEnabled(true);
			if (wifiLock == null) {
				wifiLock = wifiMgr.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "WiFiLock");
				wifiLock.setReferenceCounted(false);
				wifiLock.acquire();
			}
		}
		else {
			if ((wifiLock != null) && wifiLock.isHeld()) {
				wifiLock.release();
				wifiLock = null;
			}
	
			wifiMgr.setWifiEnabled(false);
		}
	}
	
}

