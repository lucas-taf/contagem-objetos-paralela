# Resumo de desempenho

- Matriz: 4000 x 4000, densidade 45%, semente 2026 (`tests/adicionais/grande_4000x4000.txt`), objetos = 117612
- Maquina: Apple M2 | processadores logicos: 8 | SO: Darwin 27.0.0 arm64
- Repeticoes por configuracao: 5 (1 aquecimento descartado); medida representativa: **mediana**
- Trecho medido: somente processamento (exclui leitura do arquivo); paralelo inclui criacao/join das threads e consolidacao
- Blocos no paralelo: 4 x p faixas horizontais (fila dinamica)
- Todos os resultados iguais ao sequencial: **Sim**

| Versao | Trabalhadores (p) | Tempo mediano (ms) | Min-Max (ms) | S(p) | E(p) |
|---|---:|---:|---:|---:|---:|
| Sequencial | 1 | 309.455 | 307.9-312.0 | 1.00 | 1.00 |
| Paralela | 1 | 334.405 | 329.8-337.6 | 0.93 | 0.93 |
| Paralela | 2 | 172.225 | 171.5-174.1 | 1.80 | 0.90 |
| Paralela | 4 | 93.845 | 93.5-94.0 | 3.30 | 0.82 |
| Paralela | 8 | 72.547 | 66.7-75.2 | 4.27 | 0.53 |
