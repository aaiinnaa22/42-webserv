#!/usr/bin/env php-cgi
<?php
header("Content-Type: text/plain");

$request_method = $_SERVER['REQUEST_METHOD'];

if ($request_method === "POST") {
    $post_data = file_get_contents("php://input");
	echo $post_data;
}
else 
{
	echo "NOT POST\n";
}
?>