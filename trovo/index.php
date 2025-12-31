<?php
// trovo/index.php

if (file_exists(__DIR__ . '/../trovo_secrets.php')) {
    include __DIR__ . '/../trovo_secrets.php';
}

$CLIENT_ID = $TROVO_CLIENT_ID ?? getenv('TROVO_CLIENT_ID');
$CLIENT_SECRET = $TROVO_CLIENT_SECRET ?? getenv('TROVO_CLIENT_SECRET');
$REDIRECT_URI = $TROVO_REDIRECT_URI ?? getenv('TROVO_REDIRECT_URI');

// Detect Language
$lang = substr($_SERVER['HTTP_ACCEPT_LANGUAGE'] ?? 'en', 0, 2);
$locale = ($lang === 'pt') ? 'pt' : 'en';

$messages = [
    'en' => [
        'title' => 'OBS Game Detector Plugin',
        'desc' => 'This API endpoint is used by the OBS Game Detector plugin to authenticate with Trovo securely.',
        'info_desc' => 'This page is used for plugin authentication. To download and install, use the links below:',
        'download_btn' => 'Download Plugin',
        'source_btn' => 'Source Code',
        'footer' => 'This page is not owned by, associated with, or part of <a href="https://trovo.live" target="_blank">Trovo</a>.<br>Developed by FabioZumbi12',
        'error_code' => 'Authorization code not provided.',
        'config_error' => 'Configuration Error',
        'config_missing' => 'Trovo credentials not defined.',
        'rate_limit' => 'Too many requests. Try again in 1 minute.'
    ],
    'pt' => [
        'title' => 'Plugin OBS Game Detector',
        'desc' => 'Este endpoint de API é usado pelo plugin OBS Game Detector para autenticar com a Trovo de forma segura.',
        'info_desc' => 'Esta página é usada para autenticação do plugin. Para baixar e instalar, utilize os links abaixo:',
        'download_btn' => 'Baixar Plugin',
        'source_btn' => 'Código Fonte',
        'footer' => 'Esta página não pertence, não é associada e nem faz parte oficial da <a href="https://trovo.live" target="_blank">Trovo</a>.<br>Desenvolvido por FabioZumbi12',
        'error_code' => 'Código de autorização não fornecido.',
        'config_error' => 'Erro de Configuração',
        'config_missing' => 'As credenciais da Trovo não foram definidas.',
        'rate_limit' => 'Muitas requisições. Tente novamente em 1 minuto.'
    ]
];

$t = $messages[$locale];

$svg_error = '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="#ff5252" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="15" y1="9" x2="9" y2="15"></line><line x1="9" y1="9" x2="15" y2="15"></line></svg>';
$svg_download = '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="#21b36c" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><g class="download-arrow"><polyline points="7 10 12 15 17 10"></polyline><line x1="12" y1="15" x2="12" y2="3"></line></g></svg>';

function renderHtml($title, $bodyContent, $script = '') {
    global $t, $locale;
    return "
    <!DOCTYPE html>
    <html lang='{$locale}'>
    <head>
      <meta charset='UTF-8'>
      <title>{$title}</title>
      <link href='https://fonts.googleapis.com/css2?family=Lexend:wght@300;400;500;600&display=swap' rel='stylesheet'/>
      <style>
        body {
          background-color: #18191e;
          color: #e0e6ed;
          font-family: 'Lexend', sans-serif;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          height: 100vh;
          margin: 0;
          text-align: center;
        }
        .panel {
          background-color: #1e2025;
          padding: 40px;
          border-radius: 8px;
          box-shadow: 0 4px 15px rgba(0, 0, 0, 0.5);
          max-width: 400px;
          width: 90%;
          border: 1px solid #26282d;
          display: flex;
          align-items: center;
          text-align: left;
        }
        h1 { margin-top: 0; margin-bottom: 15px; font-size: 24px; }
        p { color: #9aa5b1; margin-bottom: 0; line-height: 1.5; }
        .footer {
          margin-top: 30px;
          font-size: 12px;
          color: #5c6370;
          max-width: 400px;
          line-height: 1.5;
        }
        .footer a {
          color: #21b36c;
          text-decoration: none;
        }
        .icon-box { margin-bottom: 0; margin-right: 20px; display: flex; justify-content: center; flex-shrink: 0; }
        .download-arrow { animation: bounce 2s infinite; }
        @keyframes bounce {
          0%, 20%, 50%, 80%, 100% { transform: translateY(0); }
          40% { transform: translateY(-6px); }
          60% { transform: translateY(-3px); }
        }
      </style>
    </head>
    <body>
      <div class='panel'>
        {$bodyContent}
      </div>
      <div class='footer'>
        {$t['footer']}
      </div>
      " . ($script ? "<script>{$script}</script>" : "") . "
    </body>
    </html>
    ";
}

// Check config
if ($CLIENT_ID === 'SEU_CLIENT_ID_DA_TROVO' || empty($CLIENT_ID)) {
    if ($_SERVER['REQUEST_METHOD'] === 'GET') {
        echo renderHtml($t['config_error'], "<div class='icon-box'>{$svg_error}</div><div><h1 style='color: #ff5252'>{$t['config_error']}</h1><p>{$t['config_missing']}</p></div>");
        exit;
    } else {
        http_response_code(500);
        echo json_encode(['error' => $t['config_missing']]);
        exit;
    }
}

// Handle POST request (API)
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    header('Content-Type: application/json');

    // Rate Limiting
    $ip = $_SERVER['REMOTE_ADDR'];
    $limit = 10; // Requests
    $window = 60; // Seconds
    $rateFile = sys_get_temp_dir() . '/trovo_ratelimit_' . md5($ip);

    $requests = file_exists($rateFile) ? json_decode(file_get_contents($rateFile), true) : [];
    if (!is_array($requests)) {
        $requests = [];
    }
    $now = time();

    // Filter out requests older than the window
    $requests = array_filter($requests, function($timestamp) use ($now, $window) {
        return $timestamp > ($now - $window);
    });

    if (count($requests) >= $limit) {
        http_response_code(429);
        echo json_encode(['error' => $t['rate_limit']]);
        exit;
    }

    $requests[] = $now;
    file_put_contents($rateFile, json_encode($requests));

    // Recebe o código enviado pelo plugin
    $input = json_decode(file_get_contents('php://input'), true);
    $code = $input['code'] ?? '';

    if (empty($code)) {
        http_response_code(400);
        echo json_encode(['error' => $t['error_code']]);
        exit;
    }

    // Prepara a requisição para a Trovo
    $url = 'https://open-api.trovo.live/openplatform/exchangetoken';
    $data = [
        'client_id' => $CLIENT_ID,
        'client_secret' => $CLIENT_SECRET,
        'grant_type' => 'authorization_code',
        'code' => $code,
        'redirect_uri' => $REDIRECT_URI
    ];

    // Executa o POST usando cURL
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
    curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content_Type: application/json']);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

    $response = curl_exec($ch);
    $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    // Retorna a resposta da Trovo para o Plugin
    http_response_code($http_code);
    echo $response;
    exit;
}

// Handle GET request (Landing Page)
if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    $buttons = "
        <div style='margin-top: 20px;'>
            <a href='https://obsproject.com/forum/resources/game-detector.2260/' target='_blank' style='display: inline-block; background-color: #21b36c; color: #fff; padding: 10px 15px; border-radius: 4px; text-decoration: none; font-size: 14px; margin-right: 10px;'>{$t['download_btn']}</a>
            <a href='https://github.com/FabioZumbi12/OBSGameDetector' target='_blank' style='display: inline-block; background-color: #4f545c; color: #fff; padding: 10px 15px; border-radius: 4px; text-decoration: none; font-size: 14px;'>{$t['source_btn']}</a>
        </div>
    ";
    echo renderHtml($t['title'], "<div class='icon-box'>{$svg_download}</div><div><h1 style='color: #21b36c'>{$t['title']}</h1><p>{$t['info_desc']}</p>{$buttons}</div>");
    exit;
}
?>
