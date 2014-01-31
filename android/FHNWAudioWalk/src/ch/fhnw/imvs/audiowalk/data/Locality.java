package ch.fhnw.imvs.audiowalk.data;

import netP5.NetAddress;

/**
 * Definition of a locality with tracking.
 * @author matt
 *
 */
public class Locality {

	public int id;	
	public String name;	
	public String ssid;
	public NetAddress maxmsp;
	public float minX;
	public float maxX;
	public float minY;
	public float maxY;
	public String pdPatcherName;
}
