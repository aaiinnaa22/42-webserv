<?php

function main(): int
{
    $sock = socket_create(AF_INET, SOCK_STREAM, 0);
    $conn = socket_connect($sock, "127.0.0.1", 1234);
    if (!$conn)
	    return 1;
    $x = "GET /index.html HTTP/1.1\r\nhost: blablabla\r\nconnection: close\r\n\r\n";
    socket_write($sock,$x, strlen($x));
    sleep(1);

    $response = '';
    $out = socket_read($sock, 2048);
    
    echo "Server response:\n";
    echo $out . "\n";
    return 0;
}

exit(main());

