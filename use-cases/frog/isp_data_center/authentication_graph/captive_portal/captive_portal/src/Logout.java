
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
import org.apache.http.client.methods.HttpDelete;
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
 * Servlet implementation class Logout
 */
public class Logout extends HttpServlet {
	private static final long serialVersionUID = 1L;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public Logout() {
		super();
		// TODO Auto-generated constructor stub
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {
		
		System.out.println("Recieved a logout request from user.");
		
		HttpSession session = request.getSession();
		String user = (String) session.getAttribute("username");
		String mac = (String) session.getAttribute("mac");

		ServletOutputStream out = response.getOutputStream();
		response.setContentType("application/json");
		
		
		
		try {
			/*
			 * Sends to the Orchestrator a Delete Request for the user service graph.
			 */
			if (sendDeleteRequestToTheOrchestrator(user, mac, session) == false) {

				out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
				out.flush();
				System.out.println("Logout is not done. An error is occured");
				return;
			}
		} catch (ClientProtocolException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
			out.flush();
			System.err.println("HTTP Delete Error: " + e.getMessage());
			return;
		} catch (IOException e) {
			out.print("{\"status\":\"error\", \"accountable\": \"orchestrator\"}");
			out.flush();
			System.err.println(e.getMessage());
			throw new RuntimeException(
					"We encounter an unhandable problem in the request processing. Contact the system administrator.");
		}
		
		DeleteResponseMessage r;
		try {
			/*
			 * Sends to the OF controller a Delete OK Message.
			 */
			r = sendDeleteOKMsgToTheController(request.getRemoteAddr(), session);
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
		
		out.print("{\"status\":\"success\",\"uri\":\"http://"+((HttpServletRequest) request).getServerName()+"/login.jsp\"}");
		out.flush();
		session.setAttribute("token",null);
		System.out.println("Logout done.");

	}

	private boolean sendDeleteRequestToTheOrchestrator(String user, String mac, HttpSession session)
			throws ClientProtocolException, IOException {
		
		/*
		 * Send a a request to delete a service graph for a specific device.
		 * The user is identificated with the token obtained by keystone
		 * after the authentication.
		 */
		
		HttpClient httpClient = HttpClientBuilder.create().build();

		URL temp = new URL(
			"http",
			(String) session.getServletContext().getAttribute("orchestrator_ip"),
			Integer.parseInt((String) session.getServletContext().getAttribute("orchestrator_port")),
		 	(String) session.getServletContext().getAttribute("orchestrator_servicepath") + "/" + mac
	 	);
		System.out.println("Send request to URI resource: " + temp);
		
		HttpDelete deleteRequest = new HttpDelete(temp.toString());
		
		//putRequest.setHeader("Accept", "application/json");
		//putRequest.setHeader("X-Auth-Token", token);
		deleteRequest.setHeader("X-Auth-User", user);
		deleteRequest.setHeader("X-Auth-Pass", ((Map<String,String>) session.getServletContext().getAttribute("users")).get(user));
		deleteRequest.setHeader("X-Auth-Tenant", "public");
		HttpResponse response = httpClient.execute(deleteRequest);
		System.out.println("Orchestrator response to the delete request: "+response.getStatusLine().toString());
		if (response.getStatusLine().getStatusCode() == 200)
			return true;
		else
			return false;

	}
	

	private DeleteResponseMessage sendDeleteOKMsgToTheController(String IP_address,
			HttpSession session) throws IOException, RuntimeException {
		/* TODO: write the right comment
		 *
		 * Sends a message to the controller to tell him that the user is correctly
		 * authenticated. The controller returns the mac address of the user device
		 * and the port to which he is attached.
		 * These informations are used by the orchestrator to set a rule that bring the user traffic 
		 * on its service graph.
		 * Detail on the message return by the controller is in the class AuthResponseMessage
		 */
		System.out.println("Sending Delete_Ok to the Controller");
		MessageWithIP msg = new MessageWithIP(IP_address, null,
				Message.MsgType.Delete_OK);

		String s = sendMessageToTheController(msg, session);
		System.out.println("String received: " + s);
		JSONObject jsonObject = new JSONObject(s);

		return new DeleteResponseMessage(jsonObject);
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
