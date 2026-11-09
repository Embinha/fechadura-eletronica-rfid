// Comportamento da interface web da fechadura.
// No firmware, este mesmo código vai embutido na página gerada pelo ESP32
// (função paginaInicial() em src/esp32/fechadura_web/fechadura_web.ino).

// "Usar ultimo lido": consulta GET /ultimo e preenche o campo UID.
document.getElementById('usarUltimo').addEventListener('click', () => {
  fetch('/ultimo')
    .then(r => r.text())
    .then(t => {
      document.getElementById('uid').value = t || '';
      if (!t) alert('Nenhum cartao lido ainda');
    });
});

// Polling: como no HTTP o servidor não envia dados por conta própria,
// a página pede o log ao ESP32 (GET /log) a cada 2 segundos.
setInterval(() => {
  fetch('/log')
    .then(r => r.text())
    .then(t => { document.getElementById('log').innerHTML = t; });
}, 2000);
