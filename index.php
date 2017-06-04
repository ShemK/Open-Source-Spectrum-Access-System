<?php
$header_info = getallheaders();

/*
  This is where I test PHP commands
*/
foreach (getallheaders() as $name => $value) {
    echo "$name: $value\n";
}

$arr = array('a' => 1, 'b' => 2, 'c' => 3, 'd' => 4, 'e' => 5);

echo json_encode($arr)."\n"."<br>";

$a = array('<foo>',"'bar'",'"baz"','&blong&', "\xc3\xa9");

echo "Normal: ".  json_encode($a). "\n"."<br>";
echo "Tags: ",    json_encode($a, JSON_HEX_TAG), "\n","<br>";
echo "Apos: ",    json_encode($a, JSON_HEX_APOS), "\n"."<br>";
echo "Quot: ",    json_encode($a, JSON_HEX_QUOT), "\n"."<br>";
echo "Amp: ",     json_encode($a, JSON_HEX_AMP), "\n"."<br>";
echo "Unicode: ", json_encode($a, JSON_UNESCAPED_UNICODE), "\n"."<br>";
echo "All: ",     json_encode($a, JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP | JSON_UNESCAPED_UNICODE), "\n\n"."<br>";

$obj1 = (object)['name'=>'Shem','grades'=>array(1,2,3)];
echo json_encode($obj1)."<br>";
echo $obj1->name."<br>";
echo property_exists($obj1,'userId');

?>
