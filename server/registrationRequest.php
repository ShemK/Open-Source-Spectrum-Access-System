<?php

include_once("serversql.class.php");

$myDBHandler = new serversql();

$myDBHandler->connect();

if(isset($_POST)) {
  // get header information
  $header_info = getallheaders();

  if($header_info["Content-Type"] == "application/json") {
    // check encoding
    $char_encoding = mb_detect_encoding(file_get_contents('php://input'));
    if($char_encoding == "ASCII" || $char_encoding == "UTF-8") {
      // get raw bytes and decode them into a json object
      $jsonObj =  json_decode(file_get_contents('php://input'));
      //check the json object
      if(key($jsonObj) == "registrationRequest") {

        // store registrationRequest object
        $registrationRequestObj = $jsonObj->{'registrationRequest'};
        // check if id exists in object
        if(property_exists( $registrationRequestObj,'userId')){
          $cbsdId = $registrationRequestObj->{'userId'};
        }

        $userId = $registrationRequestObj->{'userId'};
        $fccId = $registrationRequestObj->{'fccId'};
        $cbsdSerialNumber = $registrationRequestObj->{'cbsdSerialNumber'};

        $query = "SELECT cbsdId,cbsdCategory FROM registered_cbsds where ";

        $query = $query."userId = "."'".$userId."'";
        $query = $query." and fccId = "."'".$fccId."'";


        $result = $myDBHandler->query($query);
        if($row = $myDBHandler->fetchResults($result)) {
    //      header('application/json');
          $replyObj = (object)['registrationResponse'=>'Yes','cbsdId' =>$cbsdId];
          echo json_encode($replyObj);
        } else{
     //     header('application/json');
         $replyObj = (object)['registrationResponse'=>'No'];
         echo json_encode($replyObj);
        }
      }

    }

  } else {
    $replyObj = (object)['Error'=>'JSON Object required'];
    echo json_encode($replyObj);
  }

}

?>
