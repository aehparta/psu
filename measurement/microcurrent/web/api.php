<?php


if ($_SERVER['SCRIPT_NAME'] == '/') {
    readfile(__DIR__ . '/index.html');
} else if ($_SERVER['SCRIPT_NAME'] == '/api/data') {
    $t = floatval($_GET['t']);

    $files = glob(__DIR__ . '/../*.capture');
    sort($files);

    $bdata = null;
    foreach ($files as $file) {
        $tt = floatval(basename($file, '.capture'));
        if ($tt > $t) {
            $f = fopen($file, 'r');
            // if (!flock($f, LOCK_EX)) {
            // }
            $bdata = fread($f, filesize($file));
            fclose($f);
            $t = $tt;
            break;
        }
    }

    $data = array_values(unpack('g*', $bdata));

    echo json_encode(['t' => $t, 'data' => $data]);
} else if (file_exists(__DIR__ . $_SERVER['SCRIPT_NAME'])) {
    readfile(__DIR__ . $_SERVER['SCRIPT_NAME']);
} else {
    http_response_code(404);
}
