package ch.fhnw.imvs.audiowalk;

import netP5.Logger;
import ch.fhnw.imvs.audiowalk.service.AudioWalkService;
import android.app.Application;
import android.content.Context;
import android.content.Intent;

/**
 * The application entry point.
 * @author matt
 *
 */
public class MainApplication extends Application {

	/**
	 * Called when the application is created.
	 */
    @Override
    public void onCreate() {
        
        super.onCreate();
        
        // disable netP5 internal logger
        Logger.set(Logger.ERROR, Logger.OFF);
        Logger.set(Logger.WARNING, Logger.OFF);
        Logger.set(Logger.INFO, Logger.OFF);
        Logger.set(Logger.DEBUG, Logger.OFF);
        Logger.set(Logger.PROCESS, Logger.OFF);
        Logger.set(Logger.ALL, Logger.OFF);
        
        // get context
        Context context = this.getApplicationContext();
        
        // start background services
        Intent awService = new Intent(context, AudioWalkService.class);
        context.startService(awService);
    }
}
