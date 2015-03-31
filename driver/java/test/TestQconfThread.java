import net.qihoo.qconf.Qconf;
import net.qihoo.qconf.QconfException;
import java.util.ArrayList;
import java.util.Map;
public class TestQconfThread extends Thread
{
    int pauseTime;
    public TestQconfThread(int pTime)
    {
        this.pauseTime = pTime;
    }


    public void run()
    {
        while(true)
        {
            this.doIt();
            try
            {
                Thread.sleep(pauseTime * 100);
            }
            catch (Exception e)
            {
                e.printStackTrace();      
            }
        }
    }

    public void doIt()
    {
        // ********************************Asign Idc Usage******************************
        String key = "__qconf_anchor_node";
        String idc = "corp";
        // Get Conf
        try
        {
            String value = Qconf.getConf(key, idc);
            System.out.println("Thread " + pauseTime + " conf :" + value);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get AllHost
        try
        {
            ArrayList<String> hosts = Qconf.getAllHost(key, idc);
            for(String host : hosts)
            {
                System.out.println("Thread " + pauseTime + " allhost : " + host);
            }
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get Host
        try
        {
            String host = Qconf.getHost(key, idc);
            System.out.println("Thread " + pauseTime + " host : " + host);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }

        // get Batch Conf
        try
        {
            Map<String, String> confs = Qconf.getBatchConf(key);
            for(Map.Entry<String, String> conf : confs.entrySet())
            {
                System.out.println(conf.getKey() + " : " + conf.getValue());
            }
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        
        // get Batch keys
        try
        {
            ArrayList<String> keys = Qconf.getBatchKeys(key);
            for(String one_key: keys)
            {
                System.out.println(one_key);
            }
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
            TestQconfThread tqt = new TestQconfThread(i);
            tqt.start();
        }
    }

}


