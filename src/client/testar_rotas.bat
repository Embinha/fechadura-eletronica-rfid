@echo off
chcp 65001 >nul
set IP=172.20.10.2
set LOGIN=admin:1234
set OUT=%~dp0resultado_testes_curl.txt

echo Testando o servidor da fechadura em http://%IP% ...
echo O rele vai clicar no teste 6.
echo Se o ESP32 nao responder, cada teste espera 5 s e mostra HTTP 000.

(
echo ============================================================
echo  TESTE DAS ROTAS HTTP - FECHADURA ELETRONICA
echo  Servidor: http://%IP%   Data: %date% %time%
echo ============================================================
echo.
echo [0a] GET / SEM login ^(esperado: 401 Unauthorized^)
curl.exe --connect-timeout 5 -s -i http://%IP%/
echo.
echo.
echo [0b] GET / com senha ERRADA ^(esperado: 401 e "LOGIN RECUSADO" no log^)
curl.exe --connect-timeout 5 -s -i -u admin:errada http://%IP%/
echo.
echo.
echo [0c] POST /abrir SEM login ^(esperado: 401, a porta NAO abre^)
curl.exe --connect-timeout 5 -s -i -X POST http://%IP%/abrir
echo.
echo.
echo [1] GET /  - pagina principal com login ^(esperado: 200^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -o nul -w "HTTP %%{http_code} - %%{size_download} bytes - %%{time_total} s\n" http://%IP%/
echo.
echo [2] GET /ultimo - ultimo cartao lido ^(esperado: 200^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i http://%IP%/ultimo
echo.
echo.
echo [3] POST /cadastrar - cartao de teste ^(esperado: 303^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST --data-urlencode "uid=TE ST 00 01" --data-urlencode "nome=Teste cURL" http://%IP%/cadastrar
echo.
echo [4] POST /cadastrar - mesmo cartao de novo ^(esperado: 409 Cartao ja cadastrado^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST --data-urlencode "uid=TE ST 00 01" --data-urlencode "nome=Teste cURL" http://%IP%/cadastrar
echo.
echo.
echo [5] POST /cadastrar - campos vazios ^(esperado: 400^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST -d "uid=&nome=" http://%IP%/cadastrar
echo.
echo.
echo [6] POST /abrir - abertura remota ^(esperado: 303 e o rele clica^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST http://%IP%/abrir
echo.
echo [7] GET /abrir - metodo errado ^(esperado: 404, GET nao abre a porta^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i http://%IP%/abrir
echo.
echo.
echo [8] POST /apagar - remove o cartao de teste ^(esperado: 303^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST --data-urlencode "uid=TE ST 00 01" http://%IP%/apagar
echo.
echo [9] POST /apagar - cartao inexistente ^(esperado: 404^)
curl.exe --connect-timeout 5 -u %LOGIN% -s -i -X POST --data-urlencode "uid=FF FF FF FF" http://%IP%/apagar
echo.
echo.
echo [10] GET /log - log depois dos testes ^(esperado: 200^)
curl.exe --connect-timeout 5 -u %LOGIN% -s http://%IP%/log
echo.
echo.
echo [11] Teste de carga - 20 pedidos seguidos em GET /log
for /L %%i in (1,1,20) do curl.exe --connect-timeout 5 -u %LOGIN% -s -o nul -w "pedido %%i: HTTP %%{http_code} em %%{time_total} s\n" http://%IP%/log
echo.
echo ============================================================
echo  FIM DOS TESTES
echo ============================================================
) > "%OUT%" 2>&1

type "%OUT%"
echo.
echo Resultado salvo em: %OUT%
pause
