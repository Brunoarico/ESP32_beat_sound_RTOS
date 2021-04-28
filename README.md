# ESP32_beat_sound_RTOS
Sensor de batimento cardíaco emulando som, usando sistema RTOS;

## Código

Diretórios e organização dos arquivos estão na formatação do PlatformIO.
Neste firmware são exploradas as características de sistemas RTOS, rodando em ESP32 para leitura de snsor, reprodução de som e comunicação UDP em tempo real, simultaneamente.

## Conversor

O Sample de áudio reproduzido é a 8000kHz, convertido para o formado de array por meio de um programa em python, a partir de um arquivo .mp3.
