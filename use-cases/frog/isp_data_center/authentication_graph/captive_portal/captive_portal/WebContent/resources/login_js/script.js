// LocalServer
var normalStyle = null;
var user = null;
var localserver_addr = null;
var xmlhttp;

function sendLoginRequest() {


    var success = validateLogin();

    if (success) {
        user = document.forms[0].username.value;
        /*
        var wait2 = document.getElementById("message");
        var form = document.getElementById("login-form");
        wait2.style.display = "block";
        form.style.display = "none";
        */
        /* 	loading icon */
        console.log('loading visible');
        var loading = document.getElementById("loading");
        loading.style.display = "block";

        console.log('sleep ended');
        xmlhttp = new XMLHttpRequest();
        xmlhttp.open("POST", "Login", true);
        xmlhttp.onreadystatechange = loginStatusManager;
        xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        params ="username=" + document.forms[0].username.value + "&password=" + document.forms[0].password.value;
        xmlhttp.send(params);
    }
}

function sendLogoutRequest() {

	var link = document.getElementById("link");
	var parent = link.parentNode;
	var inner = document.createElement("img");
	inner.setAttribute("src","resources/images/icons/ajax-loader.gif");
	var outer = document.createElement("div");
	outer.setAttribute("id","loader");
	outer.appendChild(inner);
	link.remove();
	parent.appendChild(outer);

    xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", "Logout");
    xmlhttp.onreadystatechange = logoutStatusManager;
    xmlhttp.send();
}

function logoutStatusManager() {
    var response = JSON.parse(xmlhttp.responseText);
    var status = response['status'];
    var accountable = response['accountable'];
    var reason = response['reason'];
   
    if (status == 'success') {
    	console.log('success');
        var uri = response['uri'];
        console.log(uri);
        window.location.replace(uri); 
    } else {
         if(accountable == 'controller openflow'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            console.log("[01] Error: some problems occurr with the SDN controller");
            errorBox.innerHTML = "Error: internal error [code: 01].";
        } else if(accountable == 'orchestrator'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            console.log("[02] Error: some problems occurr with the orchestrator");
            errorBox.innerHTML = "Error: internal error [code: 02].";
        } else {
            var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            errorBox.innerHTML = "Error: internal error [code: 03].";
        }
    }
}

function loginStatusManager() {
    var response = JSON.parse(xmlhttp.responseText);
    var status = response['status'];
    var accountable = response['accountable'];
    var reason = response['reason'];
    
    /* 	loading icon */
    console.log('loading hidden');
    var loading = document.getElementById("loading");
    loading.style.display = 'none';
    
   
    if (status == 'success') {
    	console.log('success');
        /*
         * wait2.style.display = "none";
         * 
         * var ok = document.getElementById("login_ok");
         * ok.style.display = "block";
         * var header = document.getElementById("login-intro");
         * header.style.display = "none";
        */
        var uri = response['uri'];
        console.log(uri);
        window.location.replace(uri); 
    } else {
        if (accountable == 'keystone' && reason == 'authentication') {
            var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            errorBox.innerHTML = "Login failed: username and/or password wrong.";
        } else if(accountable == 'keystone'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            errorBox.innerHTML = "Error: some problems occurr with the authentication server";
        } else if(accountable == 'controller openflow'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            console.log("[01] Error: some problems occurr with the SDN controller");
            errorBox.innerHTML = "Error: internal error [code: 01].";
        } else if(accountable == 'orchestrator'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            console.log("[02] Error: some problems occurr with the orchestrator");
            errorBox.innerHTML = "Error: internal error [code: 02].";
        } else if(accountable == 'parameters'){
        	var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            console.log("Error: some problems occurr with the orchestrator");
            errorBox.innerHTML = "Error: Wrong parameters.";
        } else {
            var errorBox = document.getElementById("error-msg");
            errorBox.style.display = 'block';
            errorBox.innerHTML = "Error: internal error [code: 03].";
        }
    }
}


function validateLogin() {
    // reset parameters
    var errorBox = document.getElementById("error-msg");
    errorBox.style.display = 'none';
    document.forms[0].username.style.border = normalStyle;
    document.forms[0].password.style.border = normalStyle;

    var username = document.forms[0].username.value;
    if (username.length == 0) {
        normalStyle = document.forms[0].username.style.border;
        document.forms[0].username.style.border = "1px solid #ff876f";
        document.forms[0].username.focus();

        errorBox.style.display = 'block';
        errorBox.innerHTML = "Error: missing username.";
        return false;
    }

    var psw = document.forms[0].password.value;
    if (psw.length == 0) {
        normalStyle = document.forms[0].password.style.border;
        document.forms[0].password.style.border = "1px solid #ff876f";
        document.forms[0].password.focus();

        errorBox.style.display = 'block';
        errorBox.innerHTML = "Error: missing password.";
        return false;
    }

    return true;
}

function submitonenter(formid,evt,thisObj) {
	evt = (evt) ? evt : ((window.event) ? window.event : "");
	if (evt)
	{
		if ( evt.keyCode==13 || evt.which==13 )
		{
			document.getElementById(formid).click();
		}
	}
}
