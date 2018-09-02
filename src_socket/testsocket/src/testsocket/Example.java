package testsocket;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.net.UnknownHostException;

public class Example {
	
	 Socket socket = null;
	    OutputStreamWriter osw;
	    DataOutputStream dos;
	    DataInputStream dis;
	    
	    
	
	void sendEntry(String entry) {
        
        try {
			dos.writeInt(entry.length());
			osw.write(entry, 0, entry.length());
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}      
        
	}
	String recvEntry() {
		int len;
		String s = null;
		try {
			len = dis.readInt();
			System.out.println(len);
			byte[] array = new byte[len];
			dis.read(array);
			s = new String(array);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		System.out.println("recv: " + s);
		return s;
	}
	
	void test() throws Exception {
	   
		System.out.println("test E");
	    String str = "register";
	    try {
	        socket = new Socket("67.218.158.111", 8881);
	        dos = new DataOutputStream(socket.getOutputStream());
	        osw =new OutputStreamWriter(socket.getOutputStream(), "UTF-8");
	        dis = new DataInputStream(socket.getInputStream());
	        
	        sendEntry(str);
	        sendEntry("1.2.3.4.5.6.7.8");
	        
	        while(true) {
		        String key = recvEntry();
		        String value = recvEntry();
		        
		        System.out.println("receive client msg: " + key + " & " + value);
	        }
	        
	        //Thread.sleep(1000*10);
	    } catch (IOException e) {
	        System.err.print(e);
	    } finally {
	        socket.close();
	    }
	    System.out.println("test X");
	}
	
	
	public static void main(String[] args) {
		Example a = new Example();
		try {
			a.test();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
}
