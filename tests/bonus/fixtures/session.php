#!/usr/bin/env php
<?php
/**
 * CGI session script — visit-counter using cookies.
 * On first visit: generates UUID session_id, sets Set-Cookie, returns visits=1.
 * On subsequent visits with valid cookie: increments visits, returns count.
 */

$SESSION_DIR = "/tmp/webserv_sessions_php";

function get_session_file($session_id) {
    global $SESSION_DIR;
    $safe = md5($session_id);
    return $SESSION_DIR . "/" . $safe . ".json";
}

function load_session($session_id) {
    $path = get_session_file($session_id);
    if (file_exists($path)) {
        $data = json_decode(file_get_contents($path), true);
        return $data;
    }
    return null;
}

function save_session($session_id, $data) {
    global $SESSION_DIR;
    if (!is_dir($SESSION_DIR)) {
        mkdir($SESSION_DIR, 0755, true);
    }
    $path = get_session_file($session_id);
    file_put_contents($path, json_encode($data));
}

function generate_uuid() {
    return sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',
        mt_rand(0, 0xffff), mt_rand(0, 0xffff),
        mt_rand(0, 0xffff),
        mt_rand(0, 0x0fff) | 0x4000,
        mt_rand(0, 0x3fff) | 0x8000,
        mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff));
}

function parse_cookies($cookie_str) {
    $cookies = array();
    if (!empty($cookie_str)) {
        $pairs = explode(";", $cookie_str);
        foreach ($pairs as $pair) {
            $pair = trim($pair);
            if (strpos($pair, "=") !== false) {
                list($k, $v) = explode("=", $pair, 2);
                $cookies[trim($k)] = trim($v);
            }
        }
    }
    return $cookies;
}

$cookie_header = isset($_SERVER["HTTP_COOKIE"]) ? $_SERVER["HTTP_COOKIE"] : "";
$cookies = parse_cookies($cookie_header);

$session_id = isset($cookies["session_id"]) ? $cookies["session_id"] : "";
$session_data = null;
$new_session = false;

if (!empty($session_id)) {
    $session_data = load_session($session_id);
}

if ($session_data === null) {
    $session_id = generate_uuid();
    $session_data = array("visits" => 0);
    $new_session = true;
}

$session_data["visits"] = $session_data["visits"] + 1;
save_session($session_id, $session_data);

$body = json_encode(array(
    "session_id" => $session_id,
    "visits" => $session_data["visits"]
));

header("Content-Type: application/json");
if ($new_session) {
    header("Set-Cookie: session_id=" . $session_id . "; Path=/session; HttpOnly");
}
header("Content-Length: " . strlen($body));
echo $body;
?>
