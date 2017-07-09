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
echo property_exists($obj1,'userId')."<br>";



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


$select_array = array("name","age");
$from_array = array("people","classes");
$where_array = array("name"=>"john","class"=>"sophomore");
$query = create_select_query($select_array,$from_array,$where_array);

echo $query."<br>";

printf("uniqid(): %s\r\n", uniqid());

/*
include_once("serverpsql.class.php");

$myDBHandler = new serverpsql();
$myDBHandler->connect();

$query = "SELECT * FROM channelinfo";
$result = $myDBHandler->query($query);
echo $myDBHandler->numRows($result);

*/
?>
