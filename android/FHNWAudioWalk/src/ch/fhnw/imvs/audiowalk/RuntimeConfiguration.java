package ch.fhnw.imvs.audiowalk;

import java.util.HashMap;
import java.util.Map;
import org.puredata.android.service.PdService;

import ch.fhnw.imvs.audiowalk.data.Locality;
import android.graphics.PointF;
import android.os.Environment;
import android.os.Handler;
import android.util.SparseArray;

/**
 * A container for all configuration properties.
 * It is initialized with default values upon instantiation and
 * may be modified at run-time.
 * @author matt
 *
 */
public class RuntimeConfiguration {
	
	public static final String DATA_DIR_NAME = "FhnwAudioWalk";
	
	public String baseDataDir;	

	public int deviceNr;
	public boolean oscEnabled;
	public int oscLocalPort;	
	public boolean pdEnabled;
	public int pdPatchHandle;
	public PdService pdService = null;
	public boolean sensorsEnabled;
	public boolean rawSensorDataToOsc;
	public boolean rawSensorDataToPd;
	public PointF currentPosition;	
	public SparseArray<Locality> localitiesById;
	public Map<String, Locality> localitiesBySsid;
	public Locality currentLocality;	
	public boolean wifiEnabled;	
	public boolean displayPosition;
	
	public Handler handler;
	
	/**
	 * Constructor.
	 * Sets default values.
	 */
	public RuntimeConfiguration() {
		loadDefaults();
		localitiesById = new SparseArray<Locality>();
		localitiesBySsid = new HashMap<String, Locality>();
	}

	/**
	 * Loads default values.
	 */
	public void loadDefaults() {
		baseDataDir = String.format("%s/%s/", Environment.getExternalStorageDirectory().getPath(), DATA_DIR_NAME);
		
		deviceNr = -1;
		oscEnabled = false;
		oscLocalPort = 9001;
		
		pdEnabled = false;
		pdPatchHandle = -1;
		handler = new Handler();
		
		sensorsEnabled = false;
		rawSensorDataToOsc = false;
		rawSensorDataToPd = false;

		currentPosition = new PointF(0.0f, 0.0f);
		
		wifiEnabled = true;		// enable by default

		displayPosition = false;

	}
	
	public void checkPositionLimit() {
		if (currentLocality != null) {
			if (currentPosition.x > currentLocality.maxX) {
				currentPosition.x = currentLocality.maxX;
			}
			else if (currentPosition.x < currentLocality.minX) {
				currentPosition.x = currentLocality.minX;
			}
			if (currentPosition.y > currentLocality.maxY) {
				currentPosition.y = currentLocality.maxY;
			}
			else if (currentPosition.y < currentLocality.minY) {
				currentPosition.y = currentLocality.minY;
			}
		}
	}
}
