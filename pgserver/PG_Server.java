package org.montsuqi;

import java.net.*;
import java.util.*;
import java.io.*;

public class PG_Server {

    public static final String VER = "1.0.7";

    String host;
    int port;
    String user;
    String pass;

    Socket s;

    PrintWriter out;
    BufferedReader in;

    Dictionary values;
    
    public PG_Server(String host, int port, String prog, String user, String pass) throws IOException {
	if (port == 0) {
	    port = 8011;
	}

	this.host = host;
	this.port = port;

	s = new Socket(host, port);
	out = new PrintWriter(s.getOutputStream());
	in = new BufferedReader(new InputStreamReader(s.getInputStream()));
	out.println("Start: " + VER + " " + user + " " + pass + " " + prog);
	String msg = in.readLine();
	if (msg.startsWith("Error: ")) {
	    System.out.println("error: " + msg.substring("Error: ".length()));
	    s.close();
	} else {
	    values = new Hashtable();
	    get_event();
	    out.println();
	}
    }
    
    public String[] get_event() throws IOException {
    	String msg = in.readLine();
	if  (msg.startsWith("Event: ")) {
	    String data = msg.substring("Event: ".length());
	    int index = data.indexOf('/');
	    if (index > 0) {
		String[] event = new String[2];
		event[0] = data.substring(0, index);
		event[1] = data.substring(index + 1);
		return event;
	    }
	    return null;
	} else {
	    System.out.println("error: connection lost ?");
	    s.close();
	    return null;
	}
    }

    private String decode(String string) {
	return string != null ? URLDecoder.decode(string) : null;
    }

    private String encode(String url) {
	return url != null ? URLEncoder.encode(url) : null;
    }

    public void event_data() {
	Enumeration e = values.keys();
	while (e.hasMoreElements()) {
	    StringBuffer buf = new StringBuffer();
	    String name = (String)(e.nextElement());
	    String value = (String)(values.get(name));
	    out.println(name + ": " + value);
	}
	out.println();
    }

    public String[] event(String event) throws IOException {
	out.println("Event: " + encode(event));
	event_data();
	return get_event();
    }

    public String[] event2(String event, String widget) throws IOException {
	out.println("Event: " + encode(event) + ":" + encode(widget));
	event_data();
	return get_event();
    }

    public String getValue(String name) throws IOException {
	out.println(name + ":");
	out.flush();
	return decode(in.readLine());
    }

    public Dictionary getValues(String name) throws IOException {
	out.println(name);
	out.flush();
	Dictionary v = new Hashtable();
	String is;
	while ((is = in.readLine()) != null) {
	    if (is.equals("")) {
		break;
	    }
	    int index = is.indexOf(": ");
	    String nam = is.substring(0, index);
	    String val = decode(is.substring(index + ": ".length()));
	    v.put(nam, val);
	}
	return v;
    }

    public void setValue(String name, String value) {
	values.put(name, value);
    }

    public void close() throws IOException {
	out.println("End");
	s.close();
    }
}
