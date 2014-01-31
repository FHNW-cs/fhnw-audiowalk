package ch.fhnw.imvs.audiowalk.activity;

import ch.fhnw.imvs.audiowalk.R;
import android.os.Bundle;
import android.preference.PreferenceActivity;

/**
 * The app settings screen.
 * @author matt
 *
 */
public class SettingsActivity extends PreferenceActivity {
	
	/**
	 * Called when the activity is created.
	 * @param savedInstanceState previously saved data 
	 */
    @SuppressWarnings("deprecation")
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.settings);
    }
}
