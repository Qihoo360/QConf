import net.qihoo.qconf.Qconf;
import net.qihoo.qconf.QconfException;
import java.util.ArrayList;
public class TestQconfMillion
{
    public void doIt()
    {
        String key = "/test_million/million";
        String default_value = "1fyJnYW1lbGFuZCI6eyJ1cmwiOiJaodHRwOlwvXC9cLz8iLCJuYW1lIjoiMzYwXHU2ZTM4XHU2MjBmIiwibWQ1IjoiMDU0MzdjMDNmZmI2YTMyNTdhOGNmMGYxNDgzNDFmZTgiLCJwbGF0Zm9ybWlkIjoiMiJ9LCJ3ZWJnYW1lIjp7ImlkIjoiMTk5NyIsImxvZ28iOm51bGwsImljb24iOiIiLCJvaW1hZ2UiOm51bGwsInNpbWFnZSI6bnVsbCwiaW1nXzUwXzUwIjoiIiwiaW1nXzY0XzY0IjoiIiwic2ltcG5hbWUiOiJjdG9sIiwibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==wibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==1fyJnYW1lbGFuZCI6eyJ1cmwiOiJaodHRwOlwvXC9cLz8iLCJuYW1lIjoiMzYwXHU2ZTM4XHU2MjBmIiwibWQ1IjoiMDU0MzdjMDNmZmI2YTMyNTdhOGNmMGYxNDgzNDFmZTgiLCJwbGF0Zm9ybWlkIjoiMiJ9LCJ3ZWJnYW1lIjp7ImlkIjoiMTk5NyIsImxvZ28iOm51bGwsImljb24iOiIiLCJvaW1hZ2UiOm51bGwsInNpbWFnZSI6bnVsbCwiaW1nXzUwXzUwIjoiIiwiaW1nXzY0XzY0IjoiIiwic2ltcG5hbWUiOiJjdG9sIiwibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==wibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==";

        // Get Conf
        try
        {
            String value = Qconf.getConf(key);
            if (value != default_value)
                System.out.println("value error:" + value);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
    }
    

    public static void main(String[] args)
    {
        for (int i = 1; i <= 100; i++)
        {
            TestQconfMillion tqt = new TestQconfMillion();
            tqt.doIt();
        }
    }

}


