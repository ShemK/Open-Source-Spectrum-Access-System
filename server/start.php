<?php


// set response header
header('Content-Type: application/json; charset=UTF-8');

include_once("json_listener.class.php");

if(isset($_POST)) {
	// get header information
	$header_info = getallheaders();

	// check if the content-type is set in the header
  	if(isset($header_info["Content-Type"])) {

  	  	if($header_info["Content-Type"] == "application/json") {
		//	echo "Received: ".file_get_contents('php://input');
    	// check encoding

    	   $char_encoding = mb_detect_encoding(file_get_contents('php://input'));
    	   if($char_encoding == "ASCII" || $char_encoding == "UTF-8") {
      		// get raw bytes and decode them into a json object
      		$jsonObj =  json_decode(file_get_contents('php://input'));
      		// create a new instance of a jsonListener object
      		$myJsonListener = new JsonListener($jsonObj);
      		$myJsonListener->processRequest();
      		echo $myJsonListener->getReply();

  		}
	 }

  } else {
  	echo "{Error:Content-Type Not Found}";
  }


}

?>
