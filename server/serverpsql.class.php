<?php

class ServerPsql {
	private $host;  // the ip-address of the database
	private $acct;  // the account of the db
	private $password; // password to the account
	private $dbName;   // name of the database
	private $dbLink;   //  returns true of the db is connected

	function ServerPsql() {
		//L$config = parse_ini_file("/usr/lib/apache2/html_configs/config.ini");
		$this->host = "127.0.0.1";
		$this->acct = "wireless";
		$this->password = "wireless";
		$this->dbName = "rem";
		$this->dbLink = false;
	}

	/*
	 *	close the connection between the SAS and REM
	*/
	function close() {
		@pg_close($dbLink);
	}

	/*
	 * create an http connection between the SAS and the database
	*/
	function connect() {
		$query = "host=".$this->host." dbname=".$this->dbName." user=".$this->acct." password=".$this->password;

		$this->dbLink= pg_connect($query) or die('connection failed: ' . pg_last_error());
		if(!$this->dbLink) {
			echo "db connection failed";
		}
	}

	/*
	 *	send a query to the database
	 *	returns an object with the results
	*/
	function query($psql) {
		if(!$this->dbLink) {
			$this->connect();
			$this->query($sql);
		} else {
			$result =  pg_query($this->dbLink,$psql);
			if(!$result) {
				echo "query failed";
				echo pg_last_error($this->dbLink);
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
			if(!$row =  pg_fetch_assoc($result)) {
				echo pg_last_error($this->dbLink);
				return false;
			} else {
				echo pg_last_error($this->dbLink);
				return $row;
			}
		}
	}
	/*
	 *	Counts the number of rows in the result
	*/
    function numRows($result){
    	$num = pg_num_rows($result);
		return $num;
	}

}

?>
