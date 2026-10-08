#!/bin/bash
# Testes funcionais: compara versao sequencial x paralela em varias configuracoes.
# Compativel com bash 3.2 (macOS) e Linux.  Saida: results/testes.txt
cd "$(dirname "$0")/.." || exit 1
SAIDA=results/testes.txt
mkdir -p results
: > "$SAIDA"
FALHAS=0

esperado_de() {   # valor esperado do enunciado para as matrizes obrigatorias
    case "$(basename "$1")" in
        ex1_*) echo 3 ;; ex2_*) echo 4 ;; ex3_*) echo 5 ;;
        ex4_*) echo 6 ;; ex5_*) echo 7 ;; *) echo "-" ;;
    esac
}
objetos() { "$@" | awk '/^Objetos:/ {print $2}'; }

# Configuracoes: "threads blocos_linhas blocos_colunas"
CONFIGS="1:1:1 2:2:1 2:1:2 4:2:2 4:3:3 3:3:3 8:4:4 2:6:6"
REPETICOES=5

echo "== Testes funcionais ($(date)) ==" | tee -a "$SAIDA"
printf "%-38s %8s %10s %-28s %s\n" "Matriz" "Esperado" "Sequencial" "Paralelo (T:BLxBC -> obj)" "Situacao" | tee -a "$SAIDA"
for f in tests/obrigatorios/*.txt tests/adicionais/*.txt; do
    esp=$(esperado_de "$f")
    seq=$(objetos ./conta-objetos-sequencial "$f")
    situ="Aprovado"
    if [ "$esp" != "-" ] && [ "$esp" != "$seq" ]; then situ="FALHOU(seq)"; fi
    resumo=""
    for cfg in $CONFIGS; do
        t=$(echo "$cfg" | cut -d: -f1); bl=$(echo "$cfg" | cut -d: -f2); bc=$(echo "$cfg" | cut -d: -f3)
        r=1
        while [ $r -le $REPETICOES ]; do        # repete para verificar determinismo
            par=$(objetos ./conta-objetos-paralelo "$f" "$t" "$bl" "$bc")
            if [ "$par" != "$seq" ]; then situ="FALHOU(T=$t ${bl}x${bc})"; fi
            r=$((r + 1))
        done
        resumo="$resumo $t:${bl}x${bc}->$par"
    done
    [ "$situ" = "Aprovado" ] || FALHAS=$((FALHAS + 1))
    printf "%-38s %8s %10s %s  %s\n" "$(basename "$f")" "$esp" "$seq" "$resumo" "$situ" | tee -a "$SAIDA"
done
echo | tee -a "$SAIDA"
echo "Cada configuracao paralela foi executada $REPETICOES vezes por matriz." | tee -a "$SAIDA"
if [ $FALHAS -eq 0 ]; then
    echo "RESULTADO: TODOS OS TESTES APROVADOS" | tee -a "$SAIDA"
else
    echo "RESULTADO: $FALHAS MATRIZ(ES) COM FALHA" | tee -a "$SAIDA"
    exit 1
fi
