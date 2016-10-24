import org.json.JSONObject;


public class Message {
	public enum MsgType {
	    Auth_OK,
	    Auth_OK_Resp,
	    Deploy_OK,
	    Deploy_OK_Resp,
	    Delete_OK,
	    Delete_OK_Resp
	}
	
	public Message(MsgType msg_type, String msg_content) {
		super();
		this.msg_type = msg_type;
		this.msg_content = msg_content;
	}
	
	public Message(JSONObject obj) {		
		super();
		this.msg_type = MsgType.valueOf((String)obj.get("Msg_Type"));
		this.msg_content = (String) obj.get("Msg_Content");
	}

	private String msg_content;
	private MsgType msg_type;

	public MsgType getMsg_type() {
		return msg_type;
	}

	public void setMsg_type(MsgType msg_type) {
		this.msg_type = msg_type;
	}

	public String getMsg_content() {
		return msg_content;
	}

	public void setMsg_content(String msg_content) {
		this.msg_content = msg_content;
	}
	
	public JSONObject getJSON()
	{
		JSONObject json = new JSONObject();
		json.put("Msg_Type", this.getMsg_type());
		json.put("Msg_Content", this.getMsg_content());
		return json;
	}
	
}

class MessageWithIP extends Message {
	public MessageWithIP(String iP_address, String msg_content, MsgType msg_type) {
		super(msg_type,msg_content);
		IP_address = iP_address;
	}
	
	public MessageWithIP(JSONObject obj) {
		
		super(MsgType.Auth_OK,(String) obj.get("Msg_Content"));
		IP_address = (String) obj.get("IP_address");
	}

	private String IP_address;

	public String getIP_address() {
		return IP_address;
	}

	public void setIP_address(String iP_address) {
		IP_address = iP_address;
	}
	
	public JSONObject getJSON()
	{
		JSONObject json = new JSONObject();
		json.put("Msg_Type", this.getMsg_type());
		json.put("Msg_Content", this.getMsg_content());
		json.put("IP_address", this.IP_address);
		return json;
	}
}


class AuthResponseMessage extends Message {
	public AuthResponseMessage(String iP_address, String mAC, String user_MAC, String user_port, String msg_content) {
		super(MsgType.Auth_OK_Resp,msg_content);
		IP_address = iP_address;
		MAC = mAC;
		this.user_MAC = user_MAC;
		this.user_port = user_port;
	}
	
	public AuthResponseMessage(JSONObject obj) {
		
		super(MsgType.Auth_OK_Resp,(String) obj.get("Msg_Content"));
		IP_address = (String) obj.get("IP_address");
		MAC = (String) obj.get("MAC");
		this.user_MAC = (String) obj.get("user_MAC");
		this.user_port = (String) obj.get("user_port");
	}

	public String getMAC() {
		return MAC;
	}

	public void setMAC(String mAC) {
		MAC = mAC;
	}

	private String IP_address,MAC,user_MAC,user_port;

	public String getIP_address() {
		return IP_address;
	}

	public void setIP_address(String iP_address) {
		IP_address = iP_address;
	}
	
	public JSONObject getJSON()
	{
		JSONObject json = new JSONObject();
		json.put("Msg_Type", this.getMsg_type());
		json.put("Msg_Content", this.getMsg_content());
		json.put("IP_address", this.IP_address);
		json.put("MAC", this.MAC);
		json.put("user_MAC", this.user_MAC);
		json.put("user_port", this.user_port);
		return json;
	}

	public String getUser_MAC() {
		return user_MAC;
	}

	public void setUser_MAC(String user_MAC) {
		this.user_MAC = user_MAC;
	}

	public String getUser_port()
	{
		return user_port;
	}

	public void setUser_port(String user_port)
	{
		this.user_port=user_port;
	}
}

class DeployResponseMessage extends Message {
	public DeployResponseMessage(String msg_content) {
		super(MsgType.Deploy_OK_Resp,msg_content);
	}
	
	public DeployResponseMessage(JSONObject obj) {
		
		super(MsgType.Deploy_OK_Resp,(String) obj.get("Msg_Content"));
	}
	
	public JSONObject getJSON()
	{
		JSONObject json = new JSONObject();
		json.put("Msg_Type", this.getMsg_type());
		json.put("Msg_Content", this.getMsg_content());
		return json;
	}
}

class DeleteResponseMessage extends Message {
	public DeleteResponseMessage(String msg_content) {
		super(MsgType.Delete_OK_Resp,msg_content);
	}
	
	public DeleteResponseMessage(JSONObject obj) {
		
		super(MsgType.Delete_OK_Resp,(String) obj.get("Msg_Content"));
	}
	
	public JSONObject getJSON()
	{
		JSONObject json = new JSONObject();
		json.put("Msg_Type", this.getMsg_type());
		json.put("Msg_Content", this.getMsg_content());
		return json;
	}
}
