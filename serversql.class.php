<?php

class ServerSql {
	private $host;  // the ip-address of the database
	private $acct;  // the account of the db
	private $password; // password to the account
	private $dbName;   // name of the database
	private $dbLink;   //  returns true of the db is connected

	function ServerSql() {
		$config = parse_ini_file("/usr/lib/apache2/html_configs/config.ini");
		$this->host = "127.0.0.1";
		$this->acct = $config['acct'];
		$this->password = $config['password'];
		$this->dbName = $config['dbName'];
		$this->dbLink = false;
	}

	/*
	 *	close the connection between the SAS and REM
	*/
	function close() {
		@mysqli_close($dbLink);
	}

	/*
	 * create an http connection between the SAS and the database
	*/
	function connect() {
		$this->dbLink = mysqli_connect($this->host,$this->acct,$this->password,$this->dbName) or exit("failed");
		if(!$this->dbLink) {
			echo "db connection failed";
		}
	}

	/*
	 *	send a query to the database
	 *	returns an object with the results
	*/
	function query($sql) {
		if(!$this->dbLink) {
			$this->connect();
			$this->query($sql);
		} else {
			$result = mysqli_query($this->dbLink,$sql);
			if(!$result) {
				echo "query failed";
				echo mysqli_error ($this->dbLink);
			}
			return $result;
		}

	}
	/*
	 *	returns an associative array from the results
	 *  that were queried.
	 *  Everytime it is called, one row is returned
	*/
	function fetchResults($result) {
		if($result==true) {
			if(!$row = mysqli_fetch_assoc($result)) {
				echo mysqli_error ($this->dbLink);
				return false;
			} else {
				echo mysqli_error ($this->dbLink);
				return $row;
			}
		}
	}
	/*
	 *	Counts the number of rows in the result
	*/
    function numRows($result){
    	$num = mysqli_num_rows($result);
		return $num;
	}

}

?>
