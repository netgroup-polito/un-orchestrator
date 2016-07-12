

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ConcurrentHashMap;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;


/**
 * Servlet Filter implementation class LoginFilter
 */
public class LoginFilter implements Filter {

    /**
     * Default constructor. 
     */
    public LoginFilter() {
        // TODO Auto-generated constructor stub
    }

	/**
	 * @see Filter#destroy()
	 */
	public void destroy() {
		// TODO Auto-generated method stub
	}

	/**
	 * @see Filter#doFilter(ServletRequest, ServletResponse, FilterChain)
	 */
	public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException, ServletException {
		   boolean authorized = false;
		   String requested_path = null;
		   if (request instanceof HttpServletRequest) 
		   {
			   
			   /* 
			    * If the requests target is a web resources, this filter is skipped
			    */
			   if (((HttpServletRequest) request).getRequestURI().matches("(.*?)\\.(js|css|jpg|png|css|js|gif)$"))
			    {		    	
			        chain.doFilter(request, response); // Just continue chain.
			        return;
			    }
			   	
			    
			   
		    	HttpSession session = ((HttpServletRequest)request).getSession();
		    	ServletContext sc = session.getServletContext();
		    	
		    	
		    	/*
		    	 * Get the token set in the Login.java Servlet when a user complete successful the authentication form
		    	 */
			    String token = ((HttpServletRequest) request).getHeader("X-Auth-Token");
			    if (token != null)
			    {	
			    	/*
			    	 * If the user is already logged we check the life of the token, if it is out of date we continue in the code flow, 
			    	 * otherwise we set the token in the session variable and set authorized to true
			    	 */
			    	
				    ConcurrentHashMap<String,Long> chm = (ConcurrentHashMap<String,Long>)sc.getAttribute("logged_users");
				    Long token_creation_timestamp = chm.get(token); 
				    if (checkSessionStatus(token_creation_timestamp, token)){
				    	authorized = true;
					    session.setAttribute("token", token);
				    }
			    }
			    else
			    {
			    	/*
			    	 *  If the user doesn't pass the token in the request
			    	 */
			    	if (((String)sc.getAttribute("captive_portal_ip")).equals(((HttpServletRequest) request).getServerName()))
			    	{		
			    		/*
			    		 *  The user's request is for the captive portal
			    		 */
			    		
			    		
			    		/*
			    		 *  Check if a token is available in the session
			    		 */
			    		token = (String) session.getAttribute("token");
			    		
	
						if (token != null){
							/*
							 * The token is already available in the session
							 */
							ConcurrentHashMap<String,Long> chm = (ConcurrentHashMap<String,Long>)sc.getAttribute("logged_users");
						    Long token_creation_timestamp = chm.get(token); 
						    if (checkSessionStatus(token_creation_timestamp, token)){
						    	//already authenticated
						    	authorized = true;
						    }							
						}
						
						
						/*
						 *  Check if the path requested by the user is already in the session
						 */
						if (session.getAttribute("requested_path") == null)
						{
							/*
							 * 	If not, we get it in the parameter target passed in the Temporary Redirect sent by this filter
							 * to the captive portal
							 */
							
							String target = ((HttpServletRequest) request).getParameter("target");
							if (target != null)
							{
								//save the requested path inside the session
								session.setAttribute("requested_path", URLDecoder.decode(target,"UTF-8"));
								System.out.println("This path is stored as target URL: " + URLDecoder.decode(((HttpServletRequest) request).getParameter("target"),"UTF-8"));
							}
						}
					}
			    	else
			    	{
			    		/*
			    		 *  The user's request is for another host, then a Temporary Redirect is sent by the Captive Portal
			    		 */
			    		requested_path = "http://"+((HttpServletRequest) request).getServerName()+((HttpServletRequest) request).getRequestURI();
						System.out.println("Requested path: " + requested_path+", a Temporary Redirect to the captive portal is sent");
						((HttpServletResponse) response).setStatus(HttpServletResponse.SC_TEMPORARY_REDIRECT);
						URL temp = new URL("http",(String)sc.getAttribute("captive_portal_ip"),80,"/Index");
						((HttpServletResponse) response).setHeader("Location", temp.toString()+"?target="+URLEncoder.encode(requested_path,"UTF-8"));
						((HttpServletResponse) response).setHeader("Connection","close");
						return;
			    	}
			    }
			    if (authorized) 
			    {
			    	/*
			    	 * The user is authenticated, continue the filter chain
			    	 */
			        chain.doFilter(request, response);
			        return;
			    }
			    else
			    {
			    	/*
			    	 * The user is not authenticated, so it is sent to the login page
			    	 */
				    String path = ((HttpServletRequest) request).getRequestURI();
				    if (path.equals("/Login"))
				    {		    	
				        chain.doFilter(request, response); // Just continue chain.
				        return;
				    }
				    else 
				    {	
				    	/*
				    	//redirect him to the Login page
				    	String paasdth=((HttpServletRequest) request).getRequestURI(); 
				    	if(paasdth.endsWith(".jpg") || paasdth.endsWith(".png") || paasdth.endsWith(".css") || paasdth.endsWith(".js") || paasdth.endsWith(".gif")){
				    		chain.doFilter(request, response);
				    		return;
				    	}
				    	*/
				    	// Maybe a redirect is more appropriate
				    	RequestDispatcher dispatch = request.getRequestDispatcher("/login.jsp");       	
						dispatch.forward(request, response);
			        	return;
				    }
	
			    }
		   }

	}
	
	private boolean checkSessionStatus(Long token_creation_timestamp, String token){
		/*
		 * Return True if the session is still valid
		 */
		/********************************************************************
	    *		   WARNING: Session expires after 5 minutes					*
	    *********************************************************************/	    
	    /* This is ignored now
	    if ((token_creation_timestamp!=null)&&((System.currentTimeMillis()-token_creation_timestamp.longValue()) < 60000 * 5))
	    {
	    	return true;
	    }
	    return false;
	    */
	    return true;
	}
	
	/**
	 * @see Filter#init(FilterConfig)
	 */
	public void init(FilterConfig fConfig) throws ServletException {

		InputStream input = fConfig.getServletContext().getResourceAsStream("/WEB-INF"+"/captive_portal.properties");
		Properties p = new Properties();
		try {
			p.load(input);
		} catch (IOException e) {
			e.printStackTrace();
			throw new RuntimeException("We encounter an unhandable problem in the request processing. Contact the system administrator.");
		}
		ServletContext sc = fConfig.getServletContext();
        if (sc != null) {
        	
        	// Loads configuration
			sc.setAttribute("captive_portal_ip", p.getProperty("captive_portal_ip"));
			sc.setAttribute("keystone_ip", p.getProperty("keystone_ip"));
			sc.setAttribute("keystone_port", p.getProperty("keystone_port"));
			sc.setAttribute("controller_ip", p.getProperty("controller_ip"));
			sc.setAttribute("controller_port", p.getProperty("controller_port"));
			sc.setAttribute("orchestrator_ip", p.getProperty("orchestrator_ip"));
			sc.setAttribute("orchestrator_port", p.getProperty("orchestrator_port"));
			sc.setAttribute("orchestrator_servicepath", p.getProperty("orchestrator_servicepath"));
			
			//TODO: Start a thread that periodically controls the map and purge the expired entries
			ConcurrentHashMap<String,Long> chm = new ConcurrentHashMap<String,Long>();
			sc.setAttribute("logged_users", chm);
			
			// Generates unique session identifier
			SessionIdentifierGenerator s = new SessionIdentifierGenerator();
			sc.setAttribute("token_generator", s);
			
			// Creates static users list
			Map<String,String> users = new HashMap<String,String>();
			users.put("user1", "password1");
			users.put("user2", "password2");
			sc.setAttribute("users", users);
        }
		
		
	}

}
