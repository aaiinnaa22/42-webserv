<?php
if (isset($_GET['number'])) {
    $number = $_GET['number'];

    if ($number == 42) {
        $message = "You entered the correct number: 42!";
    } else {
        $message = "Nope, that's not 42. You entered: $number";
    }

    echo $message;
}
?>