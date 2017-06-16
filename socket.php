<?php


/*
  All database connections are not going to be direct to the database, but rather
  through a server tcp application
  This is a test holder for the connection
*/
error_reporting(E_ALL);

echo "<h2>TCP/IP Connection</h2>\n";

/* Get the port for the WWW service. */
$service_port = 5002;

/* Get the IP address for the target host. */
//$address = "172.29.124.4";
$address = "172.29.27.117";
//$address = "172.29.102.112";
/* Create a TCP/IP socket. */
$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($socket === false) {
    echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "<br>";
} else {
    echo "OK.<br>";
}

echo "Attempting to connect to '$address' on port '$service_port'...";
$result = socket_connect($socket, $address, $service_port);
if ($result === false) {
    echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "<br>";
} else {
    echo "OK.<br>";
}

$in = "{“request_psd”:1}";
$out = '';

echo "Sending request...";
socket_write($socket, $in, strlen($in));
echo "OK.<br>";
$out = socket_read($socket,10000);
echo "Reply: ".$out."</br>";
//echo "Closing socket...";
//socket_close($socket);
echo "OK.<br><br>";

?>
