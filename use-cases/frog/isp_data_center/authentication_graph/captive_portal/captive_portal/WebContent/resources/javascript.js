var content;
var progress;
var percent;
var timer;
var instantiatioComplete = false;
var destinationURI;
var xmlHttpTimeout;
var req; //the current request
var progressTimer;

/*
 * Update frequency
 */
var updateTime = 3000;

/* 
 * Time between resources has been effectively deployed
 * and the presumed moment when users can effectively surf
 */
var waitingTime = 20000;
var updatePercentageTime = (waitingTime-2000)/10;

/*
 * Timeout captive portal request
 */
var timeoutCaptivePortal = 7000;



function status(){
	/*
	 * This function is called on load of index.jsp.
	 * It used to show the status of the instantiation process of the service graph.
	 */
	content = document.getElementById("percentage-completed");
	updateStatus();
	NProgress.set(0.08);
	timer = window.setInterval(function(){updateStatus();}, updateTime);
}

function updateStatus() {
	if ((typeof req === 'undefined')||(req.readyState == 4)){
		console.log("New request");
	    req = new XMLHttpRequest();
	    req.open("GET", "/Update", true);
	    req.onreadystatechange = checkUpdateResponse;
	    req.send(null);
	    /*
	     * If the timeout expires, it is supposed that the captive portal graph is no more 
	     * reachable.
	     */
	    xmlHttpTimeout = setTimeout(ajaxTimeout, timeoutCaptivePortal); 
	}	
}

function ajaxTimeout(){
	/*
     * If the timeout expires, it is supposed that the captive portal graph is no more 
     * reachable. So, it is called a function that render the successful page.
     */
	req.abort();
	console.log("Request aborted");
	deploy_successful();
}

function checkUpdateResponse() {
    if (req.readyState == 4) {
		console.log("readyState = 4");
    	window.clearInterval(xmlHttpTimeout);
    	if(req.status == 0) {
    		/*
    		 *  Status is 0 when either doing cross-site scripting (where access is denied) or 
    		 *  requesting a URL that is unreachable (typo, DNS issues, etc). There is an high probability 
    		 *  that we reach no more the captive portal graph, so the next request goes to timeout.
    		 */
    		console.log("Request aborted: req.status = 0");
    		return;
    	}
    	if( req.status == 502 ){
    		/*
    		 * Some proxy response 502, it is probably that the captive portal is no more reachable.
    		 * So it is called the function that render the successful page.
    		 */
    		console.log("502 returned by Captive portal, we suppose that the graph is completed and that code was returned by some proxy");
    		deploy_successful();
    		return;
    	}
        if(req.status == 200 || req.status == 201 || req.status == 202) {
        	var jsonResponse = JSON.parse(req.responseText);
        	
        	console.log("jsonResponse['instantiation_complete']: "+jsonResponse['instantiation_complete']);
        	if (jsonResponse['instantiation_complete'] == "true"){
        		/*
        		 * If instantiation_complete is true, the user reach the CP even after the graph instantiatio.
        		 * This is possible when the user is logged out and the CP session is not expires.
        		 * Setting the instantiatioComplete flag to true last 10% of frog jumping is skipped, and
        		 * is rendered only the final successfull page.
        		 */
        		instantiatioComplete = true;
        		console.log("instantiatioComplete: "+instantiatioComplete);
        	} if (typeof  destinationURI === 'undefined')
        		destinationURI = jsonResponse['requested_URI'];
        	if(typeof(jsonResponse['status']) !== undefined && jsonResponse['status'] != "CREATE_COMPLETE" && jsonResponse['status'] != "CREATE_IN_PROGRESS"){
        		console.log("Incorrect response: jsonResponse['status'] = "+jsonResponse['status']);
        		error();
        	}
        	
        	/*
        	 * Update the status bar on the top of the page.
        	 */
        	update_progressbar(jsonResponse);

        } else {
        	console.log("Wrong response status: status = "+req.status);
        	error();
        }
    }
}

function update_progressbar(jsonResponse){
	// Updates the percentage completion value
	if(jsonResponse['percentage_completed'] == 100){
		console.log("jsonResponse['percentage_completed'] == 100");
		deploy_successful();
		return;
	}
	percent = Math.round(jsonResponse['percentage_completed']);
	console.log("update_progressbar: "+percent);
	
	if(jsonResponse['percentage_completed'] < 90)
		content.textContent =percent+"%  progress,  ";
	else 
		content.textContent ="90%  progress,  ";
	
	
	//create_resorces_table();

 	// Updates progress bar 
	if(percent != 100){
		progress = percent/100;
	} else if(percent > 90){
		progress = 0.9;
	} else if(percent < 8){
		progress = 0.08;
	} else {
		/*
		 * If all the resurces are deployed the new graph is reachable,
		 * but if we obtain this information means that we are yet reaching 
		 * the captive portal graph (unusual situation, this could means that all graphs are in the same broadcast
		 * or however the ip address of the CP is reachable from the new graph)
		 */
		console.log("All resources are really instantiated");
		deploy_successful();
	}

	NProgress.set(progress);
}

function deploy_successful(){
	console.log("deploy_successful");
	window.clearInterval(timer);
	if (instantiatioComplete === true){
		console.log("The instantiation is already complete: redirect directly to successful page");
		NProgress.set(1);
		successPage();
		return;
	}
	fixed_percent = 90;
	NProgress.set(0.9);
	content.textContent ="90%  progress,  ";
	progressTimer = window.setInterval(function(){
			/*
			 * Last 10% of progress bar is piloted, with the aim of 
			 * allow the boot of all virtual machines/containers in the graph.
			 */
		    console.log("progressTimer");
		    console.log("fixed_percent: "+fixed_percent);
			if(fixed_percent == 100) return;
			else {
				fixed_percent += 1;
				content.textContent = fixed_percent+"%  progress,  ";			
				if(fixed_percent == 100){
					console.log("progress  100");
					var header = document.getElementById("header-frog3-loading");
					var top_bar = document.createElement("div");
					top_bar.id  = 'top-bar';
					document.getElementById("page").insertBefore(top_bar, header);
					header.id = 'header-frog3';
				}
				bar_progress = fixed_percent/100;
				NProgress.set(bar_progress);
			}
		}, updatePercentageTime);
	//display successful page
	waitTimerCaptivePortal = setTimeout('successPage()', waitingTime);
}

function successPage(){
	console.log("Successful page!");
	document.getElementById("message").style.display = "none";
	document.getElementById("subtitle").innerHTML = "Resources deployed";
	document.getElementById("frog-gif").style.display = "none";
	if(typeof progressTimer !== 'undefined'){
		window.clearInterval(progressTimer);
		console.log("timer progress bar ended!");
	}
	content.innerHTML = "Thanks for your patience.<br>You are now are able to access to your network.";
	if(typeof destinationURI !== 'undefined'){
		var link_div = document.createElement("div");
		link_div.id = "link";
		var link = document.createElement("a");
		link.href = destinationURI;
		link.innerHTML = "Start surfing!";
		link_div.appendChild(link);
		content.appendChild(link_div);
	}
}


function error(){
	window.clearInterval(timer);
	message = document.getElementById("message");
	message.textContent = "Deploy error!";
	frog_gif = document.getElementById("frog-img");
	frog_gif.src = "resources/images/dead_frog.jpg";
}

function create_resorces_table(){
	var table = document.createElement('table');
	for (var i = 1; i < 4; i++){
		
		for(var key in jsonResponse['resources']){
			var tr = document.createElement('tr');   

    	    var td1 = document.createElement('td');
    	    var td2 = document.createElement('td');

    	    var text1 = document.createTextNode(jsonResponse['resources'][key]['resource_name']);
    	    var text2 = document.createTextNode(jsonResponse['resources'][key]['resource_status']);

    	    td1.appendChild(text1);
    	    td2.appendChild(text2);
    	    tr.appendChild(td1);
    	    tr.appendChild(td2);

    	    table.appendChild(tr);
		}	    
	}
	content.appendChild(table);
}

