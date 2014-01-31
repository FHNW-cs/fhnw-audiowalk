package ch.fhnw.imvs.audiowalk;

import static ch.fhnw.imvs.audiowalk.Globals.DEFAULT_PD_PATCH;
import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

import java.io.File;
import java.io.IOException;

import org.puredata.core.PdBase;

import android.util.Log;

/**
 * PD helper functions.
 * @author matt
 *
 */
public class PureDataUtils {
	
	/**
	 * Loads a PD patch. If another patch was loaded it is closed first.
	 * @param config the runtime configuration
	 * @param patchName the PD patch name
	 * @throws IOException
	 */
	public static void loadPdPatch(RuntimeConfiguration config, String patchName) throws IOException {
		
		File directory = new File(config.baseDataDir);
		if (!directory.isDirectory()) {
			Log.e(LogTag, String.format("Directory does not exist."));
			return;		
		}
		
		File patchFile = new File(directory, patchName);
		PdBase.addToSearchPath(directory.getAbsolutePath());
		
		try {
			
			if (config.pdPatchHandle >= 0) {
				config.pdService.stopAudio();
				PdBase.closePatch(config.pdPatchHandle);
				Log.i(LogTag, String.format("Closed old Pd Patch."));
			}
				
			int pdPatchHandle = PdBase.openPatch(patchFile);
			if (pdPatchHandle >= 0) {
				config.pdPatchHandle = pdPatchHandle;
				config.pdService.startAudio();
				Log.i(LogTag, String.format("Loaded new Patch."));
			}
			else {
				Log.i(LogTag, String.format("Could not open new Patch"));
			}
				
		}
		catch (Exception e) {
			Log.e(LogTag, String.format("Fatal error in Pd Patch"));
		}
	}

	/**
	 * Loads the default PD patch. If another patch was loaded it is closed first.
	 * @param config the runtime configuration
	 * @throws IOException
	 */
	public static void loadPdPatch(RuntimeConfiguration config) throws IOException {
		loadPdPatch(config, DEFAULT_PD_PATCH);
	}

}
