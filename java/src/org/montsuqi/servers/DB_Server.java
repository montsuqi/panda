package org.montsuqi.servers;

import java.net.*;
import java.util.*;
import java.io.*;

public class DB_Server extends AbstractServer {

    public static final String VER = "1.1.0";

    Dictionary values;

    public DB_Server(String host, int port, String user, String pass)
	throws IOException {
	super(host, port == 0 ? 8013 : port, user, pass);
	authenticate();
	values = new Hashtable();
    }

    public String getAuthenticateString() {
	return VER + " " + user + " " + pass;
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

    public int recordops(String func, String rname, String pname, Dictionary rec) {
	out.println("Exec: " + func + ":" + rname + ":" + pname);
	exec_data(rec);
	int rc = get_event();
	getValues(rec, rname);
	return rc;
    }

}
