package ch.fhnw.imvs.audiowalk.activity;

import java.util.ArrayList;

import ch.fhnw.imvs.audiowalk.Globals;
import ch.fhnw.imvs.audiowalk.R;
import ch.fhnw.imvs.audiowalk.activity.SettingsActivity;
import ch.fhnw.imvs.audiowalk.service.AudioWalkService;
import android.os.Bundle;
import android.os.IBinder;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import static ch.fhnw.imvs.audiowalk.Globals.LogTag;

/**
 * The app's main screen.
 * @author matt
 *
 */
public class MainActivity extends Activity {
	
	private static final int SETTINGS_RESULT = 68;
	private static final int NOTIFICATION_ID = 1234;
	
	private ArrayList<String> msgLog = new ArrayList<String>();
	private boolean exitedBefore;
	
	private ListView logView;
	private ArrayAdapter<String> logAdapter;
	
	private NotificationManager notifyManager;
	
	private AudioWalkService awService = null;
	
	private ServiceConnection serviceConn = new ServiceConnection() {

		public void onServiceConnected(ComponentName className, IBinder binder) {
			awService = ((AudioWalkService.AudioWalkServiceBinder) binder).getService();
			// start background tasks
			awService.startTasks();
		}

		public void onServiceDisconnected(ComponentName className) {
			awService = null;
		}
	};
	
	private BroadcastReceiver displayLogReceiver = null;

	/**
	 * Called when the activity is created.
	 * @param savedInstanceState previously saved data 
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		if (savedInstanceState != null) {
			msgLog = savedInstanceState.getStringArrayList("msgLog");
			Log.i(LogTag, "Loaded instance state.");
		}
		
		notifyManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

		exitedBefore = false;
		
		logView = (ListView) findViewById(R.id.log_view);
		
		logAdapter = new ArrayAdapter<String>(this, R.layout.log_item, msgLog);
		logView.setAdapter(logAdapter);
		
		// a broadcast receiver to get messages from the background service to be displayed in the message log
		displayLogReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				if (intent.getAction().equals(Globals.LOG_MESSAGE_ACTION)) {
					String msg = intent.getStringExtra("message");
					if (msg != null) {
						MainActivity.this.logMessage(msg);
					}
				}
			}
			
		};
		
		this.registerReceiver(displayLogReceiver, new IntentFilter(Globals.LOG_MESSAGE_ACTION));
		
		if (!bindService(new Intent(this, AudioWalkService.class), serviceConn, Context.BIND_AUTO_CREATE)) {
			Log.e(LogTag, "Cannot bind to audio walk service.");
		}
		
		createNotificationIcon();
		
		Log.i(LogTag, "Activity created.");
	}
	
	/**
	 * Called when the activity instance state is saved.
	 * @param savedInstanceState the instance state
	 */
	@Override
	protected void onSaveInstanceState(Bundle savedInstanceState) {
		savedInstanceState.putStringArrayList("msgLog", msgLog);
		super.onSaveInstanceState(savedInstanceState);
		Log.i(LogTag, "Saved instance state.");
	}
	
	/**
	 * Called when the activity instance state is restored.
	 * @param savedInstanceState the instance state
	 */
	@Override
	protected void onRestoreInstanceState(Bundle savedInstanceState) {
		super.onRestoreInstanceState(savedInstanceState);
		if (savedInstanceState != null) {
			ArrayList<String> newLog = savedInstanceState.getStringArrayList("msgLog");
			if (newLog != null) {
				msgLog.clear();
				msgLog.addAll(newLog);
				logAdapter.notifyDataSetChanged();
			}
		}
		Log.i(LogTag, "Restored instance state.");
	}
	
	/**
	 * Called when the activity receives a new intent.
	 * @param intent the intent
	 */
	@Override
	protected void onNewIntent(Intent intent) {
		Log.i(LogTag, "New intent.");
	}
	
	/**
	 * Called when the activity is started or brought to front.
	 */
	@Override
	protected void onStart() {
		super.onStart();
		if (exitedBefore) {
			// restart background tasks if activity comes to front and was "exited" before.
			if (awService != null) {
				awService.startTasks();
				exitedBefore = false;
				logMessage("Tasks restarted after exit.");
			}
			else {
				logMessage("Cannot start background tasks because service is not bound.");
			}
		}
	}
	
	/**
	 * Called when the activity is destroyed.
	 */
	@Override
	protected void onDestroy() {
		this.unregisterReceiver(displayLogReceiver);
		unbindService(serviceConn);
		if (exitedBefore) {
			stopService(new Intent(this, AudioWalkService.class));
		}
		Log.i(LogTag, "Activity destroyed.");
		super.onDestroy();
	}

	/**
	 * Called when the action menu is created.
	 * @param menu the menu
	 * @return true
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_main, menu);
		return true;
	}

	/**
	 * Called when a menu item is selected.
	 * @param item the menu item
	 * @return a boolean
	 */
	@Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {

        	case R.id.menu_clearlog:
        		clearLog();
        		return true;
        		
        	case R.id.menu_settings:
        		Intent intent = new Intent(this, SettingsActivity.class);
        		startActivityForResult(intent, SETTINGS_RESULT);
                return true;
                
        	case R.id.menu_exit:
        		exit();  		
        		return true;
        	       
            default:
                return super.onOptionsItemSelected(item);
        }
    }
	
	/**
	 * Called when a result from an activity is received.
	 * @param requestCode the requested result when the activity was started
	 * @param resultCode the result of the activity (ok or failure)
	 * @param data intent data from the activity
	 */
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		
		if (requestCode == SETTINGS_RESULT) {
			if (awService != null) {
				if (awService.loadPreferences()) {
					logMessage("Preferences reloaded.");
				}
				else {
					logMessage("Preferences have errors.");
				}
			}
			else {
				logMessage("Cannot load preferences because service is not bound yet.");
			}
		}
		else {
			Log.e(LogTag, "Bad activity request code.");
		}
	}
	
	/**
	 * Logs a message.
	 * @param msg the message
	 */
	private void logMessage(String msg) {
		msgLog.add(msg);
		logAdapter.notifyDataSetChanged();
	}
	
	/**
	 * Clears the screen message log.
	 */
	private void clearLog() {
		msgLog.clear();
		logAdapter.notifyDataSetChanged();
	}
	
	/**
	 * Create notification bar icon to return to app.
	 */
	@SuppressWarnings("deprecation")
	private void createNotificationIcon() {
		
		Intent intent = new Intent(this, MainActivity.class);
		PendingIntent pi = PendingIntent.getActivity(this, 0, intent, Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
		
		Notification notification = new Notification.Builder(this)
			.setContentTitle(getString(R.string.app_name))
			.setContentText(getString(R.string.return_to_app))
			.setSmallIcon(R.drawable.ic_launcher)
			.setAutoCancel(false)
			.setContentIntent(pi)
			.getNotification();
		
		notification.flags = Notification.FLAG_ONGOING_EVENT | Notification.FLAG_NO_CLEAR;
		
		notifyManager.notify(NOTIFICATION_ID, notification);
	}
	
	/**
	 * Stops background services, cleans up and closes the activity.
	 * The application is not really terminated, but audio playback and communication stops.
	 */
	private void exit() {
		// Stop all background services. otherwise they keep running even if the activity is hidden.
		if (awService != null) {
			awService.stopTasks();
		}
		else {
			logMessage("Cannot stop tasks because service is not bound.");
		}
		
		exitedBefore = true;
		clearLog();
		
		// close activity
		notifyManager.cancel(NOTIFICATION_ID);
		finish();
	}
}
