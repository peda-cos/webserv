#!/usr/bin/env php
<?php
/**
 * Minimal CGI script that echoes REQUEST_METHOD and QUERY_STRING.
 */
$method = isset($_SERVER["REQUEST_METHOD"]) ? $_SERVER["REQUEST_METHOD"] : "UNKNOWN";
$query = isset($_SERVER["QUERY_STRING"]) ? $_SERVER["QUERY_STRING"] : "";
$cookie = isset($_SERVER["HTTP_COOKIE"]) ? $_SERVER["HTTP_COOKIE"] : "";

$body = "method=" . $method . "\nquery=" . $query . "\ncookie=" . $cookie . "\n";

header("Content-Type: text/plain");
header("Content-Length: " . strlen($body));
echo $body;
?>
