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
								$query = 'UPDATE registered_cbsds SET "cbsdId" = '."'".$cbsdId."'".' WHERE "fccId" = '."'".$fccId."';";
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
														$query = $this->create_update_query('registered_cbsds',array($key => $value),array("cbsdId" => $cbsdId));
														$result = $this->myDBHandler->query($query);
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
					$cbsd_table = 'cbsdInfo_' + $cbsdId;
					echo $cbsdId;							//testing
					echo $cbsd_table;			//Testing
					$query = $this->create_select_query($select_array, $cbsd_table ,$where_array);
					$result = $this->myDBHandler->query($query);
					if($row = $this->myDBHandler->fetchResults($result)) {
						if($row['available'] == 1) {
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

			$replyObj = (object) ['cbsdId'=>$cbsdId];
			$operationParam = $grantInquiryObj->{'operationParam'};
			$maxEirp = $operationParam->{'maxEirp'};
			$lowFrequency = $operationParam->{'operationalFrequencyRange'}->{'lowFrequency'};
			$highFrequency = $operationParam->{'operationalFrequencyRange'}->{'highFrequency'};

			$where_array = array('lowFrequency' => $lowFrequency,'highFrequency'=>$highFrequency);
			$query = $this->create_select_query(array('available','channelType'),'cbsd_channels',$where_array);
			$result = $this->myDBHandler->query($query);
			if($row = $this->myDBHandler->fetchResults($result)) {
				if($row['available'] == 1) {

					//echo time();
					date_default_timezone_set ('UTC');
					// add 10 seconds = 1 * 60 seconds
					$grantExpireTime = time() + (1*60/60*60);
					$grantExpireDate = $this->convertTimeToDate($grantExpireTime);
					$replyObj->{'grantExpireTime'} = $grantExpireDate;
					$replyObj->{'heartbeatInterval'} = 2;
					$replyObj->{'grantId'} = $cbsdId."_".$grantExpireDate;
					$replyObj->{'response'}->{'responseCode'} = '0';

					$attributes =  array('grantId'=>$replyObj->{'grantId'},'grantExpireTime'=>$grantExpireTime,
																'heartBeatInterval'=>$replyObj->{'heartbeatInterval'},"maxEirp"=>$maxEirp);

					$query = $this->create_insert_query('grants',$attributes);
					$result = $this->myDBHandler->query($query);

					$query = 'UPDATE cbsd_channels SET "grantId" = '."'".$replyObj->{'grantId'}."'"
					.',available = 0 WHERE "lowFrequency" = '."'".$lowFrequency."';";
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

		$replyObj = (object) [];
		if(property_exists($newHeartBeatInquiryObj, 'cbsdId')) {
			$cbsdId = $newHeartBeatInquiryObj->{'cbsdId'};
			if(property_exists($newHeartBeatInquiryObj, 'grantId')) {
				$grantId = $newHeartBeatInquiryObj->{'grantId'};

				//				$query = "SELECT * FROM grants WHERE grantId = '".$grantId."';";
				$query = $this->create_select_query('*','grants',array('grantId'=>$grantId));
				$result = $this->myDBHandler->query($query);
				if($row = $this->myDBHandler->fetchResults($result)) {

					if($row['grantState'] == 'GRANTED') {
						$query = $this->create_update_query('grants',array('grantState' => 'AUTHORIZED'),array('grantId'=>$grantId));
						$result = $this->myDBHandler->query($query);
						$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId,
						'transmitExpireTime'=>$this->convertTimeToDate($row['grantExpireTime']),
						'grantExpireTime'=>$this->convertTimeToDate($row['grantExpireTime'])];
					}
					else if($row['grantState'] == 'AUTHORIZED'){
						$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId,
						'transmitExpireTime'=>$this->convertTimeToDate($row['grantExpireTime']),
						'grantExpireTime'=>$this->convertTimeToDate($row['grantExpireTime '])];

					}
					$replyObj->{'response'}->{'responseCode'} = '0';
				}
				else {
				   $replyObj->{'response'}->{'responseCode'} = '103'; //$replyObj =  (object) ['responseCode'=>'102','Error'=>'Server Error Occured'];
					 $replyObj->{'response'}->{'Error'} = 'No grant exists';
				}
			}
		}
		return $replyObj;
	}

	/*
	Relinquishment Request
	*/

	private function relinquishmentRequest($newRelinquishmentInquiryObj) {

		if(property_exists($newRelinquishmentInquiryObj, 'cbsdId')) {

			$cbsdId = $newRelinquishmentInquiryObj->{'cbsdId'};
			if(property_exists($newRelinquishmentInquiryObj, 'grantId')) {
				$grantId = $newRelinquishmentInquiryObj->{'grantId'};
				$query = $this->create_update_query('cbsd_channels',array('available' => 1),array('grantId'=>$grantId));
				$result = $this->myDBHandler->query($query);
			}

		}
		$replyObj = (object) ['cbsdId'=>$cbsdId,'grantId'=>$grantId];
		$replyObj->{'response'}->{'responseCode'} = '0';

		return $replyObj;
	}

	/*
	Deregistration Request
	*/

	private function deregistrationRequest($newDeregistrationInquiryObj) {

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


	public function create_select_query($select_array,$from_array,$where_array) {
		$query = "SELECT ";
		if (is_array($select_array)) {
			for ($i=0; $i<count($select_array);$i++) {
				$query = $query.'"'.$select_array[$i].'"';
				if($i!=count($select_array)-1){
					$query = $query.",";
				}
			}
			$query = $query." ";
		} else {
			if($select_array=='*'){
				$query = $query.$select_array." ";
			} else {
				$query = $query.'"'.$select_array.'"'." ";
			}
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
					$query = $query.'"'.$key.'"'." = "."'".$value."'";
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

	public function create_update_query($update_table,$set_array,$where_array) {
		$query = "UPDATE ".$update_table." ";

		if($set_array!=null){
			if(is_array($set_array)) {
				$query = $query." SET ";
				$i = 0;
				foreach ($set_array as $key => $value) {
					$query = $query.'"'.$key.'"'." = "."'".$value."'";
					if($i!=count($set_array)-1){
						$query = $query.",";
					}
					$i = $i+1;
				}
			}
		}

		if($where_array!=null){
			if(is_array($where_array)) {
				$query = $query." WHERE ";
				$i = 0;
				foreach ($where_array as $key => $value) {
					$query = $query.'"'.$key.'"'." = "."'".$value."'";
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

	public function create_insert_query($insert_table,$attributes) {
		$query = "INSERT INTO ".$insert_table." ";

		if($attributes!=null){
			if(is_array($attributes)) {
				$query = $query."(";
				$keys = array_keys($attributes);
				for ($i=0; $i<count($keys);$i++) {
					$query = $query.'"'.$keys[$i].'"';
					if($i!=count($keys)-1){
						$query = $query.",";
					}
				}
				$query = $query.") VALUES(";

				$values = array_values($attributes);
				for ($i=0; $i<count($values);$i++) {
					$query = $query."'".$values[$i]."'";
					if($i!=count($values)-1){
						$query = $query.",";
					}
				}
				$query = $query.");";
			}
		}
		return $query;
	}
}
?>
