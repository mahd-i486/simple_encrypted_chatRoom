<?php
header("Content-Type: application/json; charset=utf-8");
mb_internal_encoding("UTF-8");
mb_http_output("UTF-8");
define("API_TOKEN", "super_duper_seeeeeckkroot_key"); //put the same key here as the chat.php file
$headers = getallheaders();
if (!isset($headers['X-API-TOKEN']) || $headers['X-API-TOKEN'] !== API_TOKEN) {
    http_response_code(401);
    echo json_encode(["error" => "Unauthorized"]);
    exit;
}
$conn = new mysqli("localhost", "chat", "chat", "chat");  //put mysql database Credentials here 
if ($conn->connect_error) {
    echo json_encode(["error" => "DB connection failed"]);
    exit;
}
$conn->set_charset("utf8mb4");
$conn->query("
CREATE TABLE IF NOT EXISTS test (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255) NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci
");
$method = $_SERVER['REQUEST_METHOD'];
if ($method === "GET") {
    $res = $conn->query("SELECT username, content, created_at FROM test ORDER BY id DESC LIMIT 28");
    $data = [];
    while ($row = $res->fetch_assoc()) $data[] = $row;
    echo json_encode(array_reverse($data), JSON_UNESCAPED_UNICODE);
} elseif ($method === "POST") {
    $user = $_POST['user'] ?? '';
    $content = $_POST['content'] ?? '';
    if ($user === '' || $content === '') {
        echo json_encode(["error" => "Missing fields"]);
        exit;
    }
    $stmt = $conn->prepare("INSERT INTO test(username, content) VALUES(?, ?)");
    $stmt->bind_param("ss", $user, $content);
    $stmt->execute();
    $res = $conn->query("SELECT COUNT(*) AS cnt FROM test");
    $row = $res->fetch_assoc();
    $count = (int)$row['cnt'];
    if ($count > 28) {
        $toDelete = $count - 28;
        $conn->query("DELETE FROM test ORDER BY id ASC LIMIT $toDelete");
    }
    echo json_encode(["success" => true]);
}
$conn->close();
?>
