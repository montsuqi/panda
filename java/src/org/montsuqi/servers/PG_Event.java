package org.montsuqi.servers;

public class PG_Event {

    public final String window;
    public final String widget;
    
    public PG_Event(String window, String widget) {
	this.window = window;
	this.widget = widget;
    }

    public static final PG_Event NULL_EVENT = new PG_Event("null", "null");
}
