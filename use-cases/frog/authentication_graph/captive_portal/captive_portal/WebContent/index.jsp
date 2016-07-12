

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
    "http://www.w3.org/TR/html4/loose.dtd">

<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width">
    <META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE">
    <META HTTP-EQUIV="PRAGMA" CONTENT="NO-CACHE">
    	
    <title>Logging in your FROG network</title>
    
    
	<script src='resources/nprogress.js'></script>
	<script type="text/javascript" src="resources/javascript.js"></script>
	<script type="text/javascript" src="resources/login_js/script.js"></script>
	
	<link href='resources/nprogress.css' rel='stylesheet' />
	<link href='resources/login_css/style.css' rel='stylesheet' />
  </head>
  <body onLoad = "status()">
	  <div id="page">	
	  
	  	<div id="header-frog3-loading">
				<div class="page-full-width cf inline">
					<div id="login-intro" class="fl">
						<h1 id="title">Logging in <strong>your</strong> FROG network</h1>
						<h5 id="subtitle">Deploying resources...</h5>
					</div>

				</div>
				<div id="logout" class="fr inline">
					<img src="resources/images/icons/logout-icon.png">
					<p><a onClick="sendLogoutRequest();"href="#">logout</a></p>
				</div> 
		</div>
		
		<div id="contentindex">
			<div id="info">
				<p>
					<span id="percentage-completed">0% progress,</span>
					<span id="message">Please wait</span>
					
				</p>
			</div>
			<div id="frog-gif"><img id="frog-img" src="resources/images/frog-jump-high.gif"></div>
		</div>
		
		<!-- FOOTER -->
		<div id="footer">
			<p>&copy; Copyright 2013-2016 <a href="http://www.polito.it">Politecnico di Torino</a>. All rights reserved.</p>	
		</div> <!-- end footer -->
	</div>
  </body>
</html>
