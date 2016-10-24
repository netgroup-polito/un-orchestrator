
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.URL;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.http.HttpResponse;
import org.apache.http.auth.AuthenticationException;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.HttpClientBuilder;
import org.json.JSONObject;

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;

/**
 * Servlet implementation class Login
 */
public class Login extends HttpServlet {
	private static final long serialVersionUID = 1L;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public Login() {
		super();
		// TODO Auto-generated constructor stub
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {
		
		/*
		 * This function is called by the js that handle the user login.
		 * It performs the authentication against keystone, contacts the SDN controller 
		 * to get the mac address of the user device authenticated, and finally 
		 * sends the request to the orchestrator to deploy the service graph associated
		 * to the user.
		 */
		String user = request.getParameter("username");
		String pwd = request.getParameter("password");
		String keystone_token = null;
		HttpSession session = request.getSession();
		ServletOutputStream out = response.getOutputStream();
		response.setContentType("application/json");
		
		if ((user == null) || (pwd == null) || ("".equals(pwd))
				|| ("".equals(user))) {
			// the form is empty: show an error message
			out.print("{\"status\":\"error\", \"accountable\": \"parameters\"");
			out.flush();
			System.out.println("(1) Login is not done. An error is occured");
			return;
		}
		
		try {
			/*
			 * Authontication requests through keystone  <--not supported now
			 */
			//keystone_token = authenticateThroughKeystone(user, pwd, session);
			//user is authenticated statically
			keystone_token = authenticateStatically(user, pwd, session);
			System.out.println(keystone_token);
		} /*catch (AuthenticationException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"keystone\", \"reason\": \"authentication\"}");
			out.flush();
			System.out.println("(2) Keyston auth failed");
			return;
		}*/ catch (Exception e) {
			out.print("{\"status\":\"error\", \"accountable\": \"keystone\", \"reason\": \"unknown\"}");
			out.flush();
			System.err.println(e.getMessage());
			System.out.println("Error during the authentication operation");
			return;
		}

		session.setAttribute("Keystone_token", keystone_token);
		
		/*
		 * Setting session to expires in 30 mins, however the session
		 * could be shorten if in the LoginFilter is performed a different
		 * check.
		 */
		//session.setMaxInactiveInterval(30 * 60);

		ServletContext sc = session.getServletContext();
		ConcurrentHashMap<String, Long> chm = (ConcurrentHashMap<String, Long>) sc.getAttribute("logged_users");
		SessionIdentifierGenerator token_generator = (SessionIdentifierGenerator) sc
				.getAttribute("token_generator");
		String my_token = token_generator.nextSessionId();
		while (chm.putIfAbsent(my_token, new Long(System.currentTimeMillis())) != null)
			my_token = token_generator.nextSessionId();
		session.setAttribute("token", my_token);

		
		AuthResponseMessage r;
		try {
			/*
			 * Sends to the OF controller an Auth OK Message.
			 */
			r = sendAuthOKMsgToTheController(request.getRemoteAddr(), session);
		} catch (IOException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"controller openflow\"}");
			out.flush();
			System.err.println("Socket or Input/Output stream closing error. "
					+ e.getMessage());
			return;
		} catch (RuntimeException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"controller openflow\"}");
			out.flush();
			System.err.println("Socket or Input/Output stream closing error. "
					+ e.getMessage());
			return;
		}
		
		// save mac in the session
		session.setAttribute("mac", r.getUser_MAC());
		
		try {
			/*
			 * Sends to the Orchestrator a Deploy Request for the user service graph.
			 */
			if (sendDeployRequestToTheOrchestrator(keystone_token,
					r.getIP_address(), r.getMAC(), r.getUser_MAC(), r.getUser_port(), session) == false) {
				out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
				out.flush();
				System.out.println("Login is not done. An error is occured");
				return;
			}
		} catch (ClientProtocolException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
			out.flush();
			System.err.println("HTTP Put Error: " + e.getMessage());
			return;
		} catch (IOException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
			out.flush();
			System.err.println(e.getMessage());
			throw new RuntimeException(
					"We encounter an unhandable problem in the request processing. Contact the system administrator.");
		}
		
		
		
		/*
		 * Sends Deploy Ok to Controller
		 */
		DeployResponseMessage r2;
		try {
			/*
			 * Sends to the OF controller a Deploy OK Message.
			 */
			r2 = sendDeployOKMsgToTheController(request.getRemoteAddr(), session);
		} catch (IOException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"controller openflow\"}");
			out.flush();
			System.err.println("Socket or Input/Output stream closing error. "
					+ e.getMessage());
			return;
		} catch (RuntimeException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"controller openflow\"}");
			out.flush();
			System.err.println("Socket or Input/Output stream closing error. "
					+ e.getMessage());
			return;
		}
		
		
		String requested_path = (String) session.getAttribute("requested_path");
		System.out.println("Login " + requested_path);
		out.print("{\"status\":\"success\",\"uri\":\"http://"+((HttpServletRequest) request).getServerName()+"/Index\"}");
		out.flush();
		System.out.println("Login done.");

	}

	private boolean sendDeployRequestToTheOrchestrator(String user,
			String ip_address, String mac, String user_MAC, String user_port, HttpSession session)
			throws ClientProtocolException, IOException {
		
		/*
		 * Send a a request to deploy a service graph of a specific user.
		 * The user is identificated with the token obtained by keystone
		 * after the authentication.
		 */
		
		HttpClient httpClient = HttpClientBuilder.create().build();

		URL temp = new URL("http", (String) session.getServletContext()
				.getAttribute("orchestrator_ip"),
				Integer.parseInt((String) session.getServletContext()
						.getAttribute("orchestrator_port")), (String) session.getServletContext().getAttribute("orchestrator_servicepath"));
		System.out.println("Send request to URI resource: " + temp);
		
		HttpPut putRequest = new HttpPut(temp.toString());

		StringEntity input_entity = new StringEntity("{\"session\": {\"node_id\": \"" + mac
						+ "\", \"SW_endpoint\": \"" + ip_address
						+ "\", \"device\": {\"mac\": \"" + user_MAC
						+ "\", \"port\": \"" + user_port + "\"}}}");

		
		System.out.println("{\"session\": {\"node_id\": \"" + mac
						+ "\", \"SW_endpoint\": \"" + ip_address
						+ "\", \"device\": {\"mac\": \"" + user_MAC
						+ "\", \"port\": \"" + user_port + "\"}}}");
		
		input_entity.setContentType("application/json");
		putRequest.setEntity(input_entity);
		//putRequest.setHeader("Accept", "application/json");
		//putRequest.setHeader("X-Auth-Token", token);
		putRequest.setHeader("X-Auth-User", user);
		putRequest.setHeader("X-Auth-Pass", ((Map<String,String>) session.getServletContext().getAttribute("users")).get(user));
		putRequest.setHeader("X-Auth-Tenant", "public");
		HttpResponse response = httpClient.execute(putRequest);
		System.out.println("Orchestrator response to the instantiation requests: "+response.getStatusLine().toString());
		if (response.getStatusLine().getStatusCode() == 202)
			return true;
		else
			return false;

	}
	
	private String authenticateStatically(String usr, String psw, HttpSession session) throws AuthenticationException
	{
		Map<String,String> users = (Map<String,String>) session.getServletContext().getAttribute("users");
		String pass = users.get(usr);
		if (pass==null || !pass.equals(psw))
			throw new AuthenticationException("The credentials were wrong");
		session.setAttribute("username", usr);
		return usr;
		
	}

	private String authenticateThroughKeystone(String usr, String psw,
			HttpSession session) throws AuthenticationException {
		
		/*
		 * Authenticate the user through keystone. The request returns, if the 
		 * user credentials are corrected, a token.
		 */
		
		try {
			
			HttpClient httpClient = HttpClientBuilder.create().build();

			URL temp = new URL("http", (String) session.getServletContext()
					.getAttribute("keystone_ip"),
					Integer.parseInt((String) session.getServletContext()
							.getAttribute("keystone_port")), "/v2.0/tokens");

			HttpPost postRequest = new HttpPost(temp.toString());

			StringEntity input_entity = new StringEntity(
					"{\"auth\": {\"tenantName\": \"" + usr
							+ "\", \"passwordCredentials\": {\"username\": \""
							+ usr + "\", \"password\": \"" + psw + "\"}}}");

			input_entity.setContentType("application/json");
			postRequest.setEntity(input_entity);
			postRequest.setHeader("Accept", "application/json");

			HttpResponse response = httpClient.execute(postRequest);

			if (response.getStatusLine().getStatusCode() == 200) {
				BufferedReader br = new BufferedReader(new InputStreamReader(
						(response.getEntity().getContent())));

				String json_response = br.readLine();
				if (json_response != null) {
					JSONObject jsonObject = new JSONObject(json_response);
					String r = jsonObject.getJSONObject("access")
							.getJSONObject("token").getString("id");
					if (r != null)
						return r;
				}

				throw new RuntimeException(
						"Something goes wrong with the json response");
			} else if (response.getStatusLine().getStatusCode() == 401) {
				throw new AuthenticationException("The credentials were wrong");
			} else {
				throw new RuntimeException("Failed : HTTP error code : "
						+ response.getStatusLine().getStatusCode());
			}

		} catch (ClientProtocolException e) {

			e.printStackTrace();

		} catch (IOException e) {

			e.printStackTrace();
		}
		return null;
	}

	private AuthResponseMessage sendAuthOKMsgToTheController(String IP_address,
			HttpSession session) throws IOException, RuntimeException {
		/*
		 * Sends a message to the controller to tell him that the user is correctly
		 * authenticated. The controller returns the mac address of the user device
		 * and the port to which he is attached.
		 * These informations are used by the orchestrator to set a rule that bring the user traffic 
		 * on its service graph.
		 * Detail on the message return by the controller is in the class AuthResponseMessage
		 */
		System.out.println("Sending Auth_Ok to the Controller");
		MessageWithIP msg = new MessageWithIP(IP_address, null,
				Message.MsgType.Auth_OK);

		String s = sendMessageToTheController(msg, session);
		System.out.println("String received: " + s);
		JSONObject jsonObject = new JSONObject(s);

		return new AuthResponseMessage(jsonObject);
	}
	
	private DeployResponseMessage sendDeployOKMsgToTheController(String IP_address,
			HttpSession session) throws IOException, RuntimeException {
		/*
		 * Sends a message to the controller to tell him that the user graph is correctly
		 * deployed.
		 * Detail on the message return by the controller is in the class DeployResponseMessage
		 */
		System.out.println("Sending Deploy_Ok to the Controller");
		MessageWithIP msg = new MessageWithIP(IP_address, null,
				Message.MsgType.Deploy_OK);

		String s = sendMessageToTheController(msg, session);
		System.out.println("String received: " + s);
		JSONObject jsonObject = new JSONObject(s);

		return new DeployResponseMessage(jsonObject);
	}

	private String sendMessageToTheController(MessageWithIP msg,
			HttpSession session) throws IOException {
		/*
		 *  Perform the real requests to the controller.
		 */
		
		String hostName = (String) session.getServletContext().getAttribute(
				"controller_ip");
		int portNumber = Integer.parseInt((String) session.getServletContext()
				.getAttribute("controller_port"));
		Socket mySocket = null;
		PrintWriter out = null;
		BufferedReader in = null;

		try {
			mySocket = new Socket(hostName, portNumber);
			System.err.println("After Socket");
			mySocket.setSoTimeout(10000);
			out = new PrintWriter(mySocket.getOutputStream(), true);
			System.err.println("After PrinterWriter");
			in = new BufferedReader(new InputStreamReader(
					mySocket.getInputStream()));
			System.err.println("After BufferReader");

			JSONObject jsonObject = msg.getJSON();
			out.println(jsonObject.toString());
			System.err.println(jsonObject.toString());
			String server_response;

			server_response = in.readLine();
			if (server_response != null)
				return server_response;
			else {
				System.err.println("No response from the controller");
				throw new RuntimeException(
						"We encounter an unhandable problem in the request processing. Contact the system administrator.");
			}

		} catch (java.net.SocketTimeoutException e) {
			System.err.println("Timeout expired. " + e.getMessage());
			throw new RuntimeException(
					"We encounter an unhandable problem in the request processing. Contact the system administrator.");
		} catch (Exception e) {
			System.err.println(e.getMessage());
			throw new RuntimeException(
					"We encounter an unhandable problem in the request processing. Contact the system administrator.");
		} finally {
			if (out != null)
				out.close();
			if (in != null)
				in.close();
			if (mySocket != null)
				mySocket.close();
		}

	}

}
