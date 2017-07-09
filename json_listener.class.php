<?php
include_once("serversql.class.php");
include_once("serverpsql.class.php");
class JsonListener{

	private $myJsonObj;
	private $requestType;
	private $reply;
	private $myDBHandler;
	private $serverReply;
	private	$requestResponse;
	function JsonListener($newJsonObj) {
		$this->myJsonObj = $newJsonObj;
		// reset the pointer to the first key
		@reset($this->myJsonObj)
		or exit("{'Error':'Wrong JSON Object Received'}");
		// get first key
		$this->requestType = key($this->myJsonObj);
		$this->myDBHandler = new serverpsql();
		$this->myDBHandler->connect();
		date_default_timezone_set ('UTC');
	}

	/*
	* Function to isolate each request in case mulitiple requests
	* are received in one JSON
	*/

	function processRequest() {
		$requestObj	= $this->myJsonObj->{$this->requestType};
		if (is_array($requestObj)) {
			for ($i=0; $i<count($requestObj);$i++) {
				$serverReplyArray[$i] = $this->processInternalRequest($requestObj[$i]);
			}
		} else {
			$serverReplyArray = $this->processInternalRequest($requestObj);
		}
		//echo $serverReplyArray[0];
		$this->serverReply = (object) [$this->requestResponse=>$serverReplyArray];
		return $this->serverReply;
	}
	/*
	*	Function to check the type of request in each JSON received
	*/
	private function processInternalRequest($requestObj) {
		// check the type of request
		switch ($this->requestType) {
			case 'registrationRequest':
			$this->requestResponse = 'registrationResponse';
			return $this->registrationRequest($requestObj);
			break;
			case 'spectrumInquiryRequest':
			$this->requestResponse = 'spectrumInquiryResponse';
			return $this->spectrumInquiryRequest($requestObj);
			break;
			case 'grantRequest':
			$this->requestResponse = 'grantResponse';
			return $this->grantRequest($requestObj);
			break;
			case 'heartbeatRequest':
			$this->requestResponse = 'heartbeatResponse';
			return $this->heartbeatRequest($requestObj);
			break;
			case 'relinquishmentRequest':
			$this->requestResponse = 'relinquishmentResponse';
			return $this->relinquishmentRequest($requestObj);
			break;
			case 'deregistrationRequest':
			return 0;
			break;
			default:
			return 0;
			break;
		}
	}

	/*

	Registration Request Function

	*/
	private function registrationRequest($newRegistrationRequestObj) {

		//Check for necessary data - G
		if(property_exists($newRegistrationRequestObj, 'userId'))
		{
			if(property_exists($newRegistrationRequestObj, 'fccId'))
			{
				if(property_exists($newRegistrationRequestObj, 'cbsdSerialNumber'))
				{
					$userId = $newRegistrationRequestObj->{'userId'};
					$fccId = $newRegistrationRequestObj->{'fccId'};
					$cbsdSerialNumber = $newRegistrationRequestObj->{'cbsdSerialNumber'};

					//check to see if the CBSD was blacklisted
					$where_array = array('userId' => $userId,'fccId'=>$fccId);
					$query = $this->create_select_query("*","blacklisted_cbsds",$where_array);
					$result = $this->myDBHandler->query($query);

					if($result)
					{
						if($row = $this->myDBHandler->fetchResults($result)) {
							$replyObj = (object)['response'=>(object)['responseCode'=>'101', 'responseMessage'=>'BLACKLISTED']];
						} else{

							$select_array = '*';
							$from_array = 'registered_cbsds';
							$where_array = array('userId'=>$userId,'fccId'=>$fccId);
							$query = $this->create_select_query($select_array,$from_array,$where_array);
							$result = $this->myDBHandler->query($query);

							// check if there is a result in the query with the provided parameters
							if($row = $this->myDBHandler->fetchResults($result)) {
								$cbsdId = uniqid();
								$query = "UPDATE registered_cbsds SET cbsdId = "."'".$cbsdId."'"." WHERE fccId = "."'".$fccId."';";
								$result = $this->myDBHandler->query($query);

								if($row['cbsdCategory'] == $newRegistrationRequestObj->{'cbsdCategory'}) {
									if($row['cbsdInfo'] == $newRegistrationRequestObj->{'cbsdInfo'})
									{
										if($row['cbsdSerialNumber'] == $newRegistrationRequestObj->{'cbsdSerialNumber'})
										{

											if($row['callSign'] == $newRegistrationRequestObj->{'callSign'})
											{
												$response = (object)['responseCode'=>'0','cbsdId' =>$cbsdId, 'responseMessage'=>'Registration Successful'];
												$replyObj = (object)['response' => $response];
												// update each parameter in the database
												foreach ($newRegistrationRequestObj as $key => $value) {
													if($key == 'cbsdInfo' || 'airInterface' || 'installationParam' || 'groupingParam' || 'measCapability') {
														if(is_array($value) || is_object($value)) {

															$value = json_encode($value);
														}
														$this->updateParameter($key,$value,$cbsdId,'registrationRequest');
													}

												}
												// check for given measCapability
												if(property_exists($newRegistrationRequestObj, 'measCapability')) {
													$measCapability = $newRegistrationRequestObj->{'measCapability'};
													if(is_array($measCapability)) {

														if(count($measCapability) > 1) {
															$replyObj->{'measReportConfig'} = 'EUTRA_CARRIER_RSSI_NON_TX';
														} else {
															$replyObj->{'measReportConfig'} = $measCapability[0];
														}
													}
												}
											}else{
												$replyObj = (object)['response'=>(object)['responseCode'=>'103', 'responseMessage'=>'INVALID_VALUE: invalid CBSD callSign']];
											}
										}else{
											$replyObj = (object)['response'=>(object)['responseCode'=>'103', 'responseMessage'=>'INVALID_VALUE: invalid cbsdSerialNumber']];
										}

									}else{
										$replyObj = (object)['response'=>(object)['responseCode'=>'103', 'responseMessage'=>'INVALID_VALUE: invalid cbsdInfo']];
									}

								} else {
									$replyObj = (object)['response'=>(object)['responseCode'=>'202', 'responseMessage'=>'CATEGORY_ERROR']];
								}
							} else{
								$replyObj = (object)['response'=>(object)['responseCode'=>'103']];

							}
						}
					}
				}else{
					$replyObj = (object)['response'=>(object)['responseCode'=>'200', 'responseMessage'=>'REG_PENDING: cbsdSerialNumber missing']];
				}
			}else{
				$replyObj = (object)['response'=>(object)['responseCode'=>'200', 'responseMessage'=>'REG_PENDING: fccID missing']];
			}
		}else{
			$replyObj = (object)['response'=>(object)['responseCode'=>'200', 'responseMessage'=>'REG_PENDING: userID missing']];
		}
		return $replyObj;
	}

	/*
	Spectrum Request Functions

	*/

	private function spectrumInquiryRequest($newSpectrumInquiryObj) {
		$replyObj = (object) ['responseCode'=>'102','Error'=>'Server Error Occured'];
		if(property_exists($newSpectrumInquiryObj, 'cbsdId')) {
			$cbsdId = $newSpectrumInquiryObj->{'cbsdId'};
			//$query = "SELECT cbsdId FROM registered_cbsds where cbsdId = "."'".$cbsdId."';";
			$query = $this->create_select_query("cbsdId","registered_cbsds",array("cbsdId"=>$cbsdId));
			$availableChannel = array();
			$result = $this->myDBHandler->query($query);

			if($row = $this->myDBHandler->fetchResults($result)) {

				$inquiredSpectrum =  $newSpectrumInquiryObj->{'inquiredSpectrum'};
				for($i = 0; $i < count($inquiredSpectrum);$i++) {
					$lowFrequency = $inquiredSpectrum[$i]->{'lowFrequency'};
					$highFrequency = $inquiredSpectrum[$i]->{'highFrequency'};
					//$query = "SELECT available,channelType FROM channels
					//WHERE lowFrequency = ".$lowFrequency."
					//AND highFrequency = ".$highFrequency.";";
					$select_array = array('available','channelType');
					$where_array = array('lowFrequency'=>$lowFrequency,'highFrequency'=>$highFrequency);
					$query = $this->create_select_query($select_array,'channels',$where_array);
					$result = $this->myDBHandler->query($query);
					if($row = $this->myDBHandler->fetchResults($result)) {
						if($row['available'] == TRUE) {
							array_push($availableChannel,
							(object)['lowFrequency'=>$lowFrequency,
							'highFrequency'=>$highFrequency,
							'channelType'=>$row['channelType'],
							'ruleApplied'=>'FCC_PART_96']);
						}
					}
				}
				$replyObj = (object) ['cbsdId'=>$cbsdId];
				$replyObj->{'availableChannel'} = $availableChannel;
				$replyObj->{'response'} = (object) ['responseCode' => '0'];

			} else{
				$responseData = array('cbsdId');
				$replyObj = (object) ['response'=>(object)['responseCode'=>'102','responseMessage'=>'Invalid CBSDID', 'responseData'=>$responseData]];

			}

		} else {
			$replyObj = (object) ['responseCode'=>'103','responseMessage'=>'Invalid CBSDID'];
		}
		return $replyObj;
	}

	/*
	Grant Request Functions
	*/

	private	function grantRequest($grantInquiryObj) {
		$replyObj = (object) ['responseCode'=>'102','Error'=>'Server Error Occured'];
		if(property_exists($grantInquiryObj, 'cbsdId')) {
			$cbsdId = $grantInquiryObj->{'cbsdId'};
			//echo json_encode($grantInquiryObj);
			$replyObj = (object) ['cbsdId'=>$cbsdId];
			$operationParam = $grantInquiryObj->{'operationParam'};
			$maxEirp = $operationParam->{'maxEirp'};
			$lowFrequency = $operationParam->{'operationalFrequencyRange'}->{'lowFrequency'};
			$highFrequency = $operationParam->{'operationalFrequencyRange'}->{'highFrequency'};
//$query = "SELECT available,channelType FROM channels
//			WHERE lowFrequency = ".$lowFrequency."
//			AND highFrequency = ".$highFrequency.";";
			$where_array = array('lowFrequency' => $lowFrequency,'highFrequency'=>$highFrequency);
			$query = $this->create_select_query(array('available','channelType'),'channels',$where_array);
			$result = $this->myDBHandler->query($query);
			if($row = $this->myDBHandler->fetchResults($result)) {
				if($row['available'] == TRUE) {

					//echo time();
					date_default_timezone_set ('UTC');
					// add 10 seconds = 1 * 60 seconds
					$grantExpireTime = time() + (1*10/60*60);
					$grantExpireDate = $this->convertTimeToDate($grantExpireTime);
					$replyObj->{'grantExpireTime'} = $grantExpireDate;
					$replyObj->{'heartbeatInterval'} = 2;
					$replyObj->{'grantId'} = $cbsdId."_".$grantExpireDate;
					$replyObj->{'response'}->{'responseCode'} = '0';
					$query =  "INSERT INTO grants (grantId, grantExpireTime,heartBeatInterval,maxEirp) "
					. "VALUES ( '".$replyObj->{'grantId'}. "','".$grantExpireTime."','".
					$replyObj->{'heartbeatInterval'}."','".$maxEirp."' );";
					$result = $this->myDBHandler->query($query);

					// change channel to not available
					//					$query = "UPDATE channels SET cbsdId = "."'".$cbsdId."'"
					//									.",available = 0 WHERE lowFrequency = "."'".$lowFrequency."';";
					$query = "UPDATE channels SET grantId = "."'".$replyObj->{'grantId'}."'"
					.",available = 0 WHERE lowFrequency = "."'".$lowFrequency."';";
					$result = $this->myDBHandler->query($query);

				}
			}


			//$replyObj->{}
		} else {
			$replyObj = (object) ['responseCode'=>'103','responseMessage'=>'Invalid CBSDID'];
		}

		return $replyObj;
	}


	/*
	Heartbeat Functions
	*/

	private function heartbeatRequest($newHeartBeatInquiryObj) {

		$replyObj = $newHeartBeatInquiryObj;//(object) ['responseCode'=>'102','Error'=>'Server Error Occured'];
		if(property_exists($newHeartBeatInquiryObj, 'cbsdId')) {
			$cbsdId = $newHeartBeatInquiryObj->{'cbsdId'};
			if(property_exists($newHeartBeatInquiryObj, 'grantId')) {
				$grantId = $newHeartBeatInquiryObj->{'grantId'};

//				$query = "SELECT * FROM grants WHERE grantId = '".$grantId."';";
				$query = $this->create_select_query('*','grants',array('grantId'=>$grantId));
				$result = $this->myDBHandler->query($query);
				if($row = $this->myDBHandler->fetchResults($result)) {

					if($row['grantState'] == 'GRANTED') {
						$this->updateParameter('grantState','AUTHORIZED',$grantId,'heartbeatRequest');
						$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId,
						'transmitExpireTime'=>$this->convertTimeToDate($row['grantExpireTime']),
						'grantExpireTime'=>$this->convertTimeToDate($row['grantExpireTime'])];


					}
					else if($row['grantState'] == 'AUTHORIZED'){
						$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId,
						'transmitExpireTime'=>$this->convertTimeToDate($row['grantExpireTime']),
						'grantExpireTime'=>$this->convertTimeToDate($row['grantExpireTime'])];

					}
					$replyObj->{'response'}->{'responseCode'} = '0';
				}
			}
		}
		return $replyObj;
	}

	/*
	Relinquishment Request
	*/

	private function relinquishmentRequest($newRelinquishmentInquiryObj) {
		//	echo $newRelinquishmentInquiryObj;
		if(property_exists($newRelinquishmentInquiryObj, 'cbsdId')) {

			$cbsdId = $newRelinquishmentInquiryObj->{'cbsdId'};
			if(property_exists($newRelinquishmentInquiryObj, 'grantId')) {
				$grantId = $newRelinquishmentInquiryObj->{'grantId'};
				$this->updateParameter('available',1,$grantId,'relinquishmentRequest');
			}

		}
		$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId];
		$replyObj->{'response'}->{'responseCode'} = '0';
		//	echo "yo";
		return $replyObj;
	}

	/*
	Deregistration Request
	*/

	private function deregistrationRequest($newDeregistrationInquiryObj) {

	}

	private function updateParameter($key,$value,$primaryKey,$requestType) {
		switch ($requestType) {
			case 'registrationRequest':
			$query = "UPDATE registered_cbsds SET ".$key." = "."'".$value."'"." WHERE cbsdId = "."'".$primaryKey."';";
			$result = $this->myDBHandler->query($query);
			break;
			case 'spectrumInquiryRequest':

			break;
			case 'grantRequest':

			break;
			case 'heartbeatRequest':
			if($key = 'grantState') {
				$query = "UPDATE grants SET ".$key." = "."'".$value."'"." WHERE grantId = "."'".$primaryKey."';";
				$result = $this->myDBHandler->query($query);
			}

			if($key = 'cbsdId') {

			}

			break;
			case 'relinquishmentRequest':
			$query = "UPDATE channels SET ".$key." = ".$value." WHERE grantId = '".$primaryKey."';";
			$result = $this->myDBHandler->query($query);
			break;
			case 'deregistrationRequest':

			break;
			default:

			break;
		}

	}

	public function getRequestType() {
		return $this->requestType;
	}
	public function getReply() {
		return json_encode($this->serverReply);
	}

	public function convertTimeToDate($time) {
		return date('Y-M-dTH:i:sO', $time);
	}


	function create_select_query($select_array,$from_array,$where_array) {
	  $query = "SELECT ";
	  if (is_array($select_array)) {
	    for ($i=0; $i<count($select_array);$i++) {
	      $query = $query.$select_array[$i];
	      if($i!=count($select_array)-1){
	        $query = $query.",";
	      }
	    }
	    $query = $query." ";
	  } else {
	    $query = $query.$select_array." ";
	  }

	  $query = $query." FROM ";

	  if(is_array($from_array)){
	    for ($i=0; $i<count($from_array);$i++) {
	      $query = $query.$from_array[$i];
	      if($i!=count($from_array)-1){
	        $query = $query." JOIN ";
	      }
	    }
	    $query = $query." ";
	  } else{
	    $query = $query.$from_array." ";
	  }

	  if($where_array!=null){
	    if(is_array($where_array)) {
	      $query = $query." WHERE ";
	      $i = 0;
	      foreach ($where_array as $key => $value) {
	        $query = $query.$key." = "."'".$value."'";
	        if($i!=count($where_array)-1){
	          $query = $query." AND ";
	        }
	        $i = $i+1;
	      }
	      $query = $query.";";
	    } else{
	      $query = $query.";";
	    }
	  } else{
	    $query = $query.";";
	  }
	  return $query;
	}

}
?>
