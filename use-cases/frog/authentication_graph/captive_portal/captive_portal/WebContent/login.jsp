<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
    "http://www.w3.org/TR/html4/loose.dtd">

<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0"/>  
	<META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE">
	<title>Login to your FROG network</title>
	<link href='resources/login_css/style.css' rel='stylesheet' />
	<script src="resources/login_js/script.js"></script>  
</head>
<body>

	<div id="page">	

		<!-- TOP BAR -->
		<div id="top-bar">	
		</div> <!-- end TOP BAR -->
		<div id="ranocchio">
			<!-- HEADER -->
			<div id="header">
				<div class="page-full-width cf">
					<div id="login-intro" class="fl">
						<h1>Login to <strong>your</strong> FROG network</h1>
						<h5>Enter your credentials below</h5>
					</div>
				</div>
				<div class="page-full-width cf">
					<div id="afterlogin-intro" class="fl" style="display:none;">
						<h1>Login to <strong>your</strong> FROG network</h1>
						<h5>Success!</h5>
					</div>					
				</div>
			</div>  <!-- end HEADER -->
				
			<br><br><br>
			<!-- MAIN CONTENT -->
			<div id="content">	
				<div id="form-div">
				<form action="#" method="" id="login-form">
				
					<fieldset>
						<p>
							<label for="login-username">username:</label>
							<input type="text" placeholder="Username" name="username" id="input-username" class="round full-width-input" autofocus maxlength="32" onKeyPress="submitonenter('submit',event,this)" />
						</p>
	
						<p>
							<label for="login-password">password:</label>
							<input type="password" placeholder="Password" name="password" id="input-password" class="round full-width-input" maxlength="32" onKeyPress="submitonenter('submit',event,this)"/>
						</p>
						<div id="login-res">
							
							<div id="button-div">
								<input type="button" onClick="sendLoginRequest();" name="submit" id="submit" class="button round blue image-right ic-right-arrow fr" value="LOGIN">
							</div>
							
						</div>
					</fieldset>

					<div id="loading">
					</div>

					<br/><br/>

					<div id="error-msg" class="error-box round default-width-input"> </div>
				</div>	
				</form>		
				
				<div id="message" style="display:none;" align="center"> Loading...</div>
				
				<div id="login_ok" style="display:none;" align="center">			
					<br>
					<h1 align= "center"><b>Congratulations!</b></h1>
					<br>
					<h1 align= "center">You have been successfully logged in your FROG network.</h1>
					<br>
				</div>
				
			</div> <!-- end content -->
			<br><br><br>
			<br><br><br>
		</div>
			

		<!-- FOOTER -->
		<div id="footer">
			<p>&copy; Copyright 2013-2016 <a href="http://www.polito.it">Politecnico di Torino</a>. All rights reserved.</p>	
		</div> <!-- end footer -->

	</div> <!-- end PAGE -->

</body>
</html>
