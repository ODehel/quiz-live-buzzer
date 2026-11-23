<#
.SYNOPSIS
    Fake hub to smoke-test the buzzer rendering. Replaces the real hub for the duration of a test.
.DESCRIPTION
    Listens on the address the firmware has hard-coded (192.168.1.183:3000 in src/main.cpp) and speaks
    the two things the buzzer needs from the hub (CDC 8.1, hub ws-route.ts):
      - POST /api/v1/token : answers 200 with a fixed fake token (credentials are NOT checked, never logged);
      - WebSocket /ws      : answers the first {"type":"auth"} with {"type":"auth_success"}.
    Once the buzzer is connected, an interactive menu sends one hub message per key press (render messages
    that the real hub never emits yet: question_title, question_open, buzz_locked...).
    Every message received from the buzzer (answer, buzz, auth_refresh...) is logged.
    Keys [k] [l] [n] [o] send frames the buzzer must ignore without any visible effect (US-FW-004 CA-12):
    a binary frame, a ping, an unsolicited pong and a text message split into three fragments. The binary
    frame and the fragments carry a valid {"type":"timer_end"}: if the buzzer ever read them as a text
    message, an armed buzzer would show "Trop tard". The buzzer answers a ping with a pong at the library
    level: nothing is logged for it.
    Ping and pong are written on the network stream of the WebSocket object, read by reflection (private
    field '_innerStream' observed on .NET 11.0.0, fallback '_stream'). If neither is found the log says
    "Flux reseau du WebSocket introuvable" and lists the Stream-like fields it saw: nothing was sent.
    The script stays ASCII-only (Windows PowerShell 5.1 reads a file without BOM as ANSI); accents in the
    messages are sent as JSON \u escapes.
    Run it in a real console (not the ISE: the menu reads single key presses).
.PARAMETER Prefix
    HTTP prefix to listen on. Default: http://+:3000/ (every address of the PC, so the buzzer can reach it).
    Stop the real hub first (same port).
.PARAMETER LogPath
    Optional file receiving a copy of the log (UTF-8).
.NOTES
    One-time setup, in an elevated PowerShell (or run this script elevated):
      netsh http add urlacl url=http://+:3000/ sddl=D:(A;;GX;;;WD)
    (sddl WD = Everyone whatever the Windows language: user=Everyone fails on a French Windows.)
      New-NetFirewallRule -DisplayName 'fake-hub' -Direction Inbound -Protocol TCP -LocalPort 3000 -Action Allow
    Cleanup:
      netsh http delete urlacl url=http://+:3000/
      Remove-NetFirewallRule -DisplayName 'fake-hub'
    Exit codes: 0 ok | 1 the listener could not start.
#>
param(
    [string]$Prefix = 'http://+:3000/',
    [string]$LogPath = ''
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

function Write-Log([string]$Text) {
    $line = '{0:HH:mm:ss} {1}' -f (Get-Date), $Text
    Write-Host $line
    if ($LogPath) { Add-Content -Path $LogPath -Value $line -Encoding UTF8 }
}

# 243 uppercase characters, no accent: the longest realistic title of the 250-character guarantee.
$longTitle = ('QUEL ANIMAL MARIN POSSEDE TROIS COEURS ET DU SANG BLEU ' * 5).Substring(0, 243)

$menu = @(
    @{ Key = '1'; Label = 'question_title : Question 5, titre court'
       Json = '{"type":"question_title","question_index":4,"title":"Combien de c\u0153urs a une pieuvre ?"}' },
    @{ Key = '2'; Label = 'question_title : Question 7, guillemets francais'
       Json = '{"type":"question_title","question_index":6,"title":"Qui a dit \u00ab Je pense, donc je suis \u00bb ?"}' },
    @{ Key = '3'; Label = 'question_title : Question 100, titre de 243 majuscules'
       Json = ('{{"type":"question_title","question_index":99,"title":"{0}"}}' -f $longTitle) },
    @{ Key = '4'; Label = 'question_open  : titre seul (SPEED)'
       Json = '{"type":"question_open","title":"Quel animal a trois c\u0153urs ?"}' },
    @{ Key = '5'; Label = 'timer_tick     : 25 s'
       Json = '{"type":"timer_tick","remaining_seconds":25}' },
    @{ Key = '6'; Label = 'buzz_locked    : Armelle a buzze'
       Json = '{"type":"buzz_locked","buzzer_username":"Armelle"}' },
    @{ Key = '7'; Label = 'buzz_accepted'
       Json = '{"type":"buzz_accepted"}' },
    @{ Key = '8'; Label = 'buzz_invalidated'
       Json = '{"type":"buzz_invalidated"}' },
    @{ Key = '9'; Label = 'buzz_unlocked'
       Json = '{"type":"buzz_unlocked"}' },
    @{ Key = 'a'; Label = 'answer_received'
       Json = '{"type":"answer_received"}' },
    @{ Key = 'b'; Label = 'game_resumed'
       Json = '{"type":"game_resumed"}' },
    @{ Key = 'c'; Label = 'timer_end'
       Json = '{"type":"timer_end"}' },
    @{ Key = 'd'; Label = 'question_choices : Paris, Lyon, Nice, Lille'
       Json = '{"type":"question_choices","choices":["Paris","Lyon","Nice","Lille"]}' },
    @{ Key = 'e'; Label = 'auth_success (renvoi)'
       Json = '{"type":"auth_success","username":"buzzer-test","role":"buzzer","expires_in":3600}' },
    @{ Key = 'f'; Label = 'question_choices : une proposition de 36 caracteres'
       Json = '{"type":"question_choices","choices":["Paris","Cr\u00e8me br\u00fbl\u00e9e au caramel de Normandie","Nice","Lille"]}' },
    @{ Key = 'g'; Label = 'question_result : MCQ, bonne reponse B, reponse du joueur C'
       Json = '{"type":"question_result","correct_answer":"B","correct":false,"player_answer":"C"}' },
    @{ Key = 'h'; Label = 'question_result : MCQ, bonne reponse B, pas de reponse du joueur'
       Json = '{"type":"question_result","correct_answer":"B","correct":false}' },
    @{ Key = 'i'; Label = 'question_result : SPEED, valide'
       Json = '{"type":"question_result","correct_answer":"Berlin","correct":true}' },
    @{ Key = 'j'; Label = 'question_result : SPEED, rate'
       Json = '{"type":"question_result","correct_answer":"Berlin","correct":false}' },
    @{ Key = 'k'; Label = 'CA-12 : trame BINAIRE (octets de {"type":"timer_end"}), a ignorer'
       Action = { Send-WsBinary '{"type":"timer_end"}' } },
    @{ Key = 'l'; Label = 'CA-12 : PING (trame de controle 0x9), a ignorer'
       Action = { Send-WsRawFrame ([byte[]](0x89, 0x04, 0x70, 0x69, 0x6E, 0x67)) 'PING (4 octets "ping")' } },
    @{ Key = 'n'; Label = 'CA-12 : PONG non sollicite (trame de controle 0xA), a ignorer'
       Action = { Send-WsRawFrame ([byte[]](0x8A, 0x04, 0x70, 0x6F, 0x6E, 0x67)) 'PONG non sollicite (4 octets "pong")' } },
    @{ Key = 'o'; Label = 'CA-12 : texte FRAGMENTE en 3 morceaux (timer_end), perdu sans bruit'
       Action = { Send-WsFragmented @('{"type":"tim', 'er_end"', '}') } }
)

function Show-Menu {
    Write-Host ''
    Write-Host 'Message a envoyer au buzzer :'
    foreach ($item in $menu) { Write-Host ('  [{0}] {1}' -f $item.Key, $item.Label) }
    Write-Host '  [m] reafficher le menu   [q] quitter'
    Write-Host ''
}

function Send-Http($response, [int]$status, [string]$json) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $response.StatusCode = $status
    $response.ContentType = 'application/json'
    $response.ContentLength64 = $bytes.Length
    $response.OutputStream.Write($bytes, 0, $bytes.Length)
    $response.Close()
}

function Send-Ws([string]$json) {
    if (-not $script:socket -or $script:socket.State -ne [System.Net.WebSockets.WebSocketState]::Open) {
        Write-Log 'Aucun buzzer connecte : message non envoye.'
        return
    }
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $segment = [System.ArraySegment[byte]]::new($bytes)
    $script:socket.SendAsync($segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [System.Threading.CancellationToken]::None).Wait()
    Write-Log "-> $json"
}

function Test-WsOpen {
    if (-not $script:socket -or $script:socket.State -ne [System.Net.WebSockets.WebSocketState]::Open) {
        Write-Log 'Aucun buzzer connecte : message non envoye.'
        return $false
    }
    return $true
}

# One binary frame (US-FW-004 CA-12).
function Send-WsBinary([string]$content) {
    if (-not (Test-WsOpen)) { return }
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($content)
    $segment = [System.ArraySegment[byte]]::new($bytes)
    $script:socket.SendAsync($segment, [System.Net.WebSockets.WebSocketMessageType]::Binary, $true, [System.Threading.CancellationToken]::None).Wait()
    Write-Log "-> (binaire) $content"
}

# One text message split into several WebSocket fragments: every part but the last is sent with
# endOfMessage = $false (FIN = 0), so the buzzer receives a text frame then continuation frames.
function Send-WsFragmented([string[]]$parts) {
    if (-not (Test-WsOpen)) { return }
    for ($i = 0; $i -lt $parts.Count; $i++) {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($parts[$i])
        $segment = [System.ArraySegment[byte]]::new($bytes)
        $isLast = ($i -eq $parts.Count - 1)
        $script:socket.SendAsync($segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $isLast, [System.Threading.CancellationToken]::None).Wait()
        Write-Log ('-> (fragment {0}/{1}, fin={2}) {3}' -f ($i + 1), $parts.Count, $isLast, $parts[$i])
        Start-Sleep -Milliseconds 200
    }
}

# A control frame (ping, pong) written straight on the network stream: System.Net.WebSockets has no public
# way to send a ping on demand. Server frames are never masked. The stream is read by reflection from a
# private field of the WebSocket object, searched along the type chain (ServerWebSocket, then WebSocketBase).
# Observed on .NET 11.0.0 (probe of 2026-11-23): the field is '_innerStream' (there is no '_stream'); the
# old name '_stream' stays as a fallback. NOT yet run against a buzzer: if no field is found, the script
# lists the Stream-like fields it saw and sends nothing.
function Get-WsNetworkStream {
    $flags = [System.Reflection.BindingFlags]'Instance,NonPublic,Public'
    $names = @('_innerStream', '_stream')
    $seen = New-Object System.Collections.Generic.List[string]
    foreach ($name in $names) {
        $type = $script:socket.GetType()
        while ($type) {
            $field = $type.GetField($name, $flags)
            if ($field) {
                $value = $field.GetValue($script:socket)
                if ($value -is [System.IO.Stream]) {
                    return [pscustomobject]@{ Stream = $value; Field = ('{0}.{1}' -f $type.Name, $name); Seen = '' }
                }
            }
            $type = $type.BaseType
        }
    }
    $type = $script:socket.GetType()
    while ($type) {
        foreach ($field in $type.GetFields([System.Reflection.BindingFlags]'Instance,NonPublic,Public,DeclaredOnly')) {
            if ([System.IO.Stream].IsAssignableFrom($field.FieldType) -or $field.Name -match 'tream') {
                $seen.Add(('{0}.{1} ({2})' -f $type.Name, $field.Name, $field.FieldType.Name))
            }
        }
        $type = $type.BaseType
    }
    return [pscustomobject]@{ Stream = $null; Field = ''; Seen = ($seen -join ', ') }
}

function Send-WsRawFrame([byte[]]$frame, [string]$label) {
    if (-not (Test-WsOpen)) { return }
    $found = Get-WsNetworkStream
    if (-not $found.Stream) {
        $seen = if ($found.Seen) { $found.Seen } else { '(aucun)' }
        Write-Log ("Flux reseau du WebSocket introuvable (champs '_innerStream' et '_stream' absents ou sans Stream) : $label non envoye. Champs de type Stream vus : $seen")
        return
    }
    try {
        $found.Stream.Write($frame, 0, $frame.Length)
        $found.Stream.Flush()
        Write-Log "-> ($label, ecrit sur $($found.Field))"
    }
    catch {
        Write-Log "Ecriture de la trame impossible sur $($found.Field) : $label non envoye ($($_.Exception.Message))"
    }
}

function Close-Socket {
    if ($script:socket) {
        try { $script:socket.Dispose() } catch { }
    }
    $script:socket = $null
    $script:receiveTask = $null
}

function Handle-Context($context) {
    $request = $context.Request
    $path = $request.Url.AbsolutePath
    if ($request.IsWebSocketRequest -and $path -eq '/ws') {
        $accept = $context.AcceptWebSocketAsync([System.Management.Automation.Language.NullString]::Value)   # no sub-protocol: [string]$null would become '' and be refused
        $accept.Wait()
        Close-Socket
        $script:socket = $accept.Result.WebSocket
        Write-Log 'WebSocket /ws : connexion ouverte.'
        return
    }
    if ($request.HttpMethod -eq 'POST' -and $path -eq '/api/v1/token') {
        $reader = [System.IO.StreamReader]::new($request.InputStream, [System.Text.Encoding]::UTF8)
        [void]$reader.ReadToEnd()   # credentials are read and dropped: never checked, never logged
        Write-Log 'POST /api/v1/token : 200 (identifiants non verifies).'
        Send-Http $context.Response 200 '{"token":"fake-hub-token","token_type":"Bearer","expires_in":3600}'
        return
    }
    Write-Log ('{0} {1} : 404' -f $request.HttpMethod, $path)
    Send-Http $context.Response 404 '{"status":404,"error":"NOT_FOUND","message":"Fake hub: unknown route."}'
}

function Poll-Socket {
    if (-not $script:socket) { return }
    if (-not $script:receiveTask) {
        $segment = [System.ArraySegment[byte]]::new($script:buffer)
        $script:receiveTask = $script:socket.ReceiveAsync($segment, [System.Threading.CancellationToken]::None)
    }
    if (-not $script:receiveTask.IsCompleted) { return }
    if ($script:receiveTask.IsFaulted -or $script:receiveTask.IsCanceled) {
        Write-Log 'WebSocket /ws : connexion perdue.'
        Close-Socket
        return
    }
    $result = $script:receiveTask.Result
    $script:receiveTask = $null
    if ($result.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) {
        Write-Log 'WebSocket /ws : fermee par le buzzer.'
        Close-Socket
        return
    }
    $text = [System.Text.Encoding]::UTF8.GetString($script:buffer, 0, $result.Count)
    $type = ''
    try { $type = ($text | ConvertFrom-Json).type } catch { }
    if ($type -eq 'auth') {
        Write-Log '<- auth (jeton non verifie)'
        Send-Ws '{"type":"auth_success","username":"buzzer-test","role":"buzzer","expires_in":3600}'
    }
    else {
        Write-Log "<- $text"
    }
}

$script:socket = $null
$script:receiveTask = $null
$script:buffer = New-Object byte[] 8192

$listener = [System.Net.HttpListener]::new()
$listener.Prefixes.Add($Prefix)
try { $listener.Start() }
catch {
    Write-Host "Impossible de demarrer l'ecoute sur $Prefix : $($_.Exception.Message)"
    Write-Host "Le vrai hub est-il arrete ? La reservation d'URL (netsh http add urlacl) est-elle faite ? Voir .NOTES."
    exit 1
}

Write-Log "Faux hub a l'ecoute sur $Prefix (Ctrl+C ou [q] pour quitter)."
Show-Menu

$contextTask = $listener.GetContextAsync()
try {
    $running = $true
    while ($running) {
        if ($contextTask.Wait(50)) {
            $context = $contextTask.Result
            $contextTask = $listener.GetContextAsync()
            try { Handle-Context $context }
            catch { Write-Log "Erreur de traitement : $($_.Exception.Message)" }
        }
        Poll-Socket
        while ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true).KeyChar.ToString().ToLowerInvariant()
            if ($key -eq 'q') { $running = $false; break }
            if ($key -eq 'm') { Show-Menu; continue }
            $item = $menu | Where-Object { $_.Key -eq $key } | Select-Object -First 1
            if ($item -and $item.Action) { & $item.Action }
            elseif ($item) { Send-Ws $item.Json }
        }
    }
}
finally {
    Close-Socket
    $listener.Stop()
    $listener.Close()
    Write-Log 'Faux hub arrete.'
}
