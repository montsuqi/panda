package org.montsuqi.servers;

import java.net.*;
import java.util.*;
import java.io.*;

public abstract class AbstractServer {
    public static final String VER = "1.1.1";

    protected String host;
    protected int port;

    protected String user;
    protected String pass;
    
    protected Socket s;

    protected PrintWriter out;
    protected BufferedReader in;

    protected AbstractServer(String host, int port, String user, String pass)
	throws IOException {
	this.host = host;
	this.port = port;
	this.user = user;
	this.pass = pass;
	s = new Socket(host, port);
	out = new PrintWriter(s.getOutputStream());
	in = new BufferedReader(new InputStreamReader(s.getInputStream()));
    }

    protected void authenticate() throws IOException {
	String auth = getAuthenticateString();
	out.println(auth);
	out.flush();
	String msg = in.readLine();
	if (msg == null) {
	    System.out.println("error: no data from socket.");
	    s.close();
	} else if (msg.startsWith("Error: ")) {
	    System.out.println("error: " + msg.substring("Error: ".length()));
	    s.close();
	}
    }

    public void close() {
	out.println("End");
	try {
	    s.close();
	} catch (IOException e) {
	    e.printStackTrace();
	}
    }

    public void finalize() throws Throwable {
	close();
    }
    
    protected abstract String getAuthenticateString();

    // Utilitiy methods
    protected static String decode(String string) {
	return string != null ? URLDecoder.decode(string) : null;
    }

    protected static String encode(String url) {
	return url != null ? URLEncoder.encode(url) : null;
    }
}
