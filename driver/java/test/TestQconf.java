import net.qihoo.qconf.Qconf;
import net.qihoo.qconf.QconfException;
import java.util.ArrayList;
import java.util.Map;
public class TestQconf
{

    public static void main(String[] args)
    {
        String key_conf = "/demo";
        String key_host = "/demo/hosts";
        String idc = "corp";
        System.out.println(Qconf.version());

        // ********************************Basic Usage******************************
        // Get Conf
        try
        {
            String value = Qconf.getConf(key_conf);
            System.out.println(value);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get AllHost
        try
        {
            ArrayList<String> hosts = Qconf.getAllHost(key_host);
            for(String host : hosts)
            {
                System.out.println(host);
            }
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get Host
        try
        {
            String host = Qconf.getHost(key_host);
            System.out.println(host);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }

        // get Batch Conf
        try
        {
            Map<String, String> confs = Qconf.getBatchConf(key_conf);
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
            ArrayList<String> keys = Qconf.getBatchKeys(key_conf);
            for(String one_key: keys)
            {
                System.out.println(one_key);
            }
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        

        System.out.println(" ************************************************************");
        // ********************************Asign Idc Usage******************************
        // Get Conf
        try
        {
            String value = Qconf.getConf(key_conf, idc);
            System.out.println(value);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get AllHost
        try
        {
            ArrayList<String> hosts = Qconf.getAllHost(key_host, idc);
            for(String host : hosts)
            {
                System.out.println(host);
            }
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        // get Host
        try
        {
            String host = Qconf.getHost(key_host, idc);
            System.out.println(host);
        }
        catch(QconfException e)
        {
            e.printStackTrace();
        }
        
        // get Batch Conf
        try
        {
            Map<String, String> confs = Qconf.getBatchConf(key_conf, idc);
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
            ArrayList<String> keys = Qconf.getBatchKeys(key_conf, idc);
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
}


