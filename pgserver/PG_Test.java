import java.text.*;
import java.util.*;
import org.montsuqi.*;

public class PG_Test {
    public static void main(String[] args) {

	PG_Server server = new PG_Server("localhost",0,"panda:sample","ogochan",";suPer00");

	server.setValue("list.fixed1.key.value","ogo%");

	PG_Event ev = server.event("Search");

	System.out.println("ev.window = " + ev.window);
	System.out.println("ev.widget = " + ev.widget);

	int count = Integer.parseInt(server.getValue("list.fixed1.clist1.count"));
	System.out.println("count = " + count);

	int i = 0;
	Format f = new MessageFormat("[{0}][{1}][{2}][{3}]");
	while (i < count) {
	    String v1 = server.getValue("list.fixed1.clist1.item[" + i + "].value1");
	    String v2 = server.getValue("list.fixed1.clist1.item[" + i + "].value2");
	    String v3 = server.getValue("list.fixed1.clist1.item[" + i + "].value3");
	    String v4 = server.getValue("list.fixed1.clist1.item[" + i + "].value4");
	    System.out.println(f.format(new Object[] { v1, v2, v3, v4 }));
	    i++;
	}

	Dictionary values = server.getValues("list.fixed1.clist1.item");
	Enumeration keys = values.keys();
	while (keys.hasMoreElements()) {
	    String name = (String)(keys.nextElement());
	    String value = (String)(values.get(name));
	    System.out.println(name + ": " + value);
	}
	server.close();
    }
}
