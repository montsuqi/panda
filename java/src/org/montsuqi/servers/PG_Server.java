package org.montsuqi.servers;

import java.net.*;
import java.util.*;
import java.io.*;

public class PG_Server extends AbstractServer {

    public static final String VER = "1.0.7";

    String prog;

    Dictionary values;

    public PG_Server(String host, int port, String prog, String user, String pass)
	throws IOException {
	super(host, port == 0 ? 8011 : port, user, pass);
	this.prog = prog;
	authenticate();
	values = new Hashtable();
	get_event();
	out.println();
    }

    protected String getAuthenticateString() {
	return "Start: " + VER + " " + user + " " + pass + " " + prog;
    }

    public PG_Event event(String event) {
	out.println("Event: " + encode(event));
	event_data();
	out.flush();
	return get_event();
    }

    public PG_Event event2(String event, String widget) {
	out.println("Event: " + encode(event) + ":" + encode(widget));
	event_data();
	out.flush();
	return get_event();
    }

    public String getValue(String name) {
	out.println(name + ":");
	out.flush();
	try {
	    return decode(in.readLine());
	} catch (IOException e) {
	    e.printStackTrace();
	    try {
		s.close();
	    } catch (IOException e2) {
		e2.printStackTrace();
	    }
	    return null;
	}
    }

    public Dictionary getValues(String name) {
	out.println(name);
	out.flush();
	Dictionary v = new Hashtable();
	String is;
	try {
	    while ((is = in.readLine()) != null) {
		if (is.length() == 0) {
		    break;
		}
		int index = is.indexOf(": ");
		if (index > 0) {
		    String nam = is.substring(0, index);
		    String val = decode(is.substring(index + ": ".length()));
		    v.put(nam, val);
		}
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	    try {
		s.close();
	    } catch (IOException e2) {
		e2.printStackTrace();
	    }
	}
	return v;
    }

    public void setValue(String name, String value) {
	values.put(name, value);
    }

    public void close() {
	out.println("End");
	try {
	    s.close();
	} catch (IOException e) {
	    e.printStackTrace();
	}
    }

    private PG_Event get_event() {
	try {
	    String msg = in.readLine();
	    if  (msg != null && msg.startsWith("Event: ")) {
		String data = msg.substring("Event: ".length());
		int index = data.indexOf('/');
		if (index > 0) {
		    String widget = data.substring(0, index);
		    String window = data.substring(index + 1);
		    return new PG_Event(widget, window);
		}
		return PG_Event.NULL_EVENT;
	    } else {
		System.out.println("error: connection lost ?");
		s.close();
		return PG_Event.NULL_EVENT;
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	    try {
		s.close();
	    } catch (IOException e2) {
		e2.printStackTrace();
	    }
	    return PG_Event.NULL_EVENT;
	}
    }

    private void event_data() {
	Enumeration keys = values.keys();
	while (keys.hasMoreElements()) {
	    String name = (String)(keys.nextElement());
	    String value = (String)(values.get(name));
	    out.println(name + ": " + value);
	}
	out.println();
    }
}
