package org.montsuqi;

import java.net.*;
import java.util.*;
import java.io.*;

public class DB_Server {

    public static final String VER = "1.1.0";

    String host;
    int port;
    
    Socket s;

    PrintWriter out;
    BufferedReader in;

    Dictionary values;

    public DB_Server(String host, int port, String user, String pass) {
	if (port == 0) {
	    port = 8013;
	}

	this.host = host;
	this.port = port;

	try {
	    s = new Socket(host, port);
	    out = new PrintWriter(s.getOutputStream());
	    in = new BufferedReader(new InputStreamReader(s.getInputStream()));

	    out.println(VER + " " + user + " " + pass);
	    out.flush();
	    String msg = in.readLine();
	    if (msg == null) {
		System.out.println("error: no data from socket.");
		s.close();
	    } else if  (msg.startsWith("Error: ")) {
		System.out.println("error: " + msg.substring("Error: ".length()));
		s.close();
	    } else {
		values = new Hashtable();
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	}
    }
    
    public int get_event() {
	try {
	    String msg = in.readLine();
	    if (msg != null && msg.startsWith("Exec: ")) {
		return Integer.parseInt(msg.substring("Exec: ".length()));
	    } else {
		System.out.println("error: connection lost ?");
		s.close();
		return -1;
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	    try {
		s.close();
	    } catch (IOException e2) {
		e2.printStackTrace();
	    }
	    return -1;
	}
    }
	
    public void exec_data(Dictionary rec) {
	Enumeration e = rec.keys();
	while (e.hasMoreElements()) {
	    String name = (String)e.nextElement();
	    String value = encode((String)rec.get(name));
	    out.println(name + " " + value);
	    out.println();
	}
    }
    
    public int dbops(String func) {
	out.println("Exec: " + func);
	out.println();
	int rc = get_event();
	out.println();
	return rc;
    }

    public Dictionary getValues(Dictionary rec, String name) {
	out.println(name);
	out.flush();

	String is;
	try {
	    while ((is = in.readLine()) != null) {
		System.out.println("is = [" + is + "]");
		if (is.length() == 0) {
		    break;
		}
		int index = is.indexOf(": ");
		if (index > 0) {
		    String nam = is.substring(0, index);
		    String val = decode(is.substring(index + ": ".length()));
		    rec.put(nam, val);
		}
	    }
	    out.println();
	} catch (IOException e) {
	    e.printStackTrace();
	}
	return rec;
    }

    public int recordops(String func, String rname, String pname, Dictionary rec) {
	out.println("Exec: " + func + ":" + rname + ":" + pname);
	exec_data(rec);
	int rc = get_event();
	getValues(rec, rname);
	return rc;
    }

    public void close() {
	out.println("End");
	try {
	    s.close();
	} catch (IOException e) {
	    e.printStackTrace();
	}
    }
    
    // Utilitiy methods
    private static String decode(String string) {
	return string != null ? URLDecoder.decode(string) : null;
    }

    private static String encode(String url) {
	return url != null ? URLEncoder.encode(url).toUpperCase() : null;
    }
}
