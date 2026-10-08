#!/bin/bash
# Avaliacao de desempenho: sequencial x paralelo (1, 2, 4, 8 threads).
# Uso: ./scripts/benchmark.sh [linhas colunas densidade semente repeticoes]
# Saidas: results/medicoes.csv (dados brutos) e results/resumo_desempenho.md
cd "$(dirname "$0")/.." || exit 1
L=${1:-4000}; C=${2:-4000}; D=${3:-45}; S=${4:-2026}; REP=${5:-5}
THREADS="1 2 4 8"
MAT=tests/adicionais/grande_${L}x${C}.txt
CSV=results/medicoes.csv
MD=results/resumo_desempenho.md
mkdir -p results

if [ ! -f "$MAT" ]; then
    echo "Gerando matriz $L x $C (densidade $D%, semente $S)..."
    ./gera-matriz "$L" "$C" "$D" "$S" > "$MAT" || exit 1
fi

echo "matriz,linhas,colunas,versao,trabalhadores,repeticao,tempo_ms,objetos" > "$CSV"
campo() { awk -v k="$1" '$1==k":" {print $2}'; }

echo "Aquecimento (descartado)..."
./conta-objetos-sequencial "$MAT" > /dev/null

r=1
while [ $r -le $REP ]; do
    out=$(./conta-objetos-sequencial "$MAT")
    echo "grande,$L,$C,sequencial,1,$r,$(echo "$out" | campo Tempo_ms),$(echo "$out" | campo Objetos)" >> "$CSV"
    for t in $THREADS; do
        out=$(./conta-objetos-paralelo "$MAT" "$t" $((t * 4)) 1)
        echo "grande,$L,$C,paralela,$t,$r,$(echo "$out" | campo Tempo_ms),$(echo "$out" | campo Objetos)" >> "$CSV"
    done
    echo "  repeticao $r/$REP concluida"
    r=$((r + 1))
done

# Resumo: mediana, min-max, aceleracao S(p)=Tseq/Tpar e eficiencia E(p)=S(p)/p
mediana() {   # $1 = versao, $2 = trabalhadores
    awk -F, -v v="$1" -v t="$2" 'NR>1 && $4==v && $5==t {print $7}' "$CSV" | sort -n |
        awk '{a[NR]=$1} END {if (NR%2) print a[(NR+1)/2]; else printf "%.3f\n", (a[NR/2]+a[NR/2+1])/2}'
}
faixa() {
    awk -F, -v v="$1" -v t="$2" 'NR>1 && $4==v && $5==t {print $7}' "$CSV" | sort -n |
        awk 'NR==1{mi=$1} {ma=$1} END {printf "%.1f-%.1f", mi, ma}'
}
OBJ=$(awk -F, 'NR==2 {print $8}' "$CSV")
CORRETO=$(awk -F, -v o="$OBJ" 'NR>1 && $8!=o {e=1} END {print (e ? "Nao" : "Sim")}' "$CSV")
TSEQ=$(mediana sequencial 1)
if command -v sysctl >/dev/null 2>&1 && sysctl -n hw.ncpu >/dev/null 2>&1; then
    NCPU=$(sysctl -n hw.ncpu); CPU=$(sysctl -n machdep.cpu.brand_string 2>/dev/null)
else
    NCPU=$(nproc 2>/dev/null); CPU=$(awk -F: '/model name/ {print $2; exit}' /proc/cpuinfo 2>/dev/null)
fi
{
    echo "# Resumo de desempenho"
    echo
    echo "- Matriz: ${L} x ${C}, densidade ${D}%, semente ${S} (\`$MAT\`), objetos = ${OBJ}"
    echo "- Maquina: ${CPU} | processadores logicos: ${NCPU} | SO: $(uname -sr) $(uname -m)"
    echo "- Repeticoes por configuracao: ${REP} (1 aquecimento descartado); medida representativa: **mediana**"
    echo "- Trecho medido: somente processamento (exclui leitura do arquivo); paralelo inclui criacao/join das threads e consolidacao"
    echo "- Blocos no paralelo: 4 x p faixas horizontais (fila dinamica)"
    echo "- Todos os resultados iguais ao sequencial: **${CORRETO}**"
    echo
    echo "| Versao | Trabalhadores (p) | Tempo mediano (ms) | Min-Max (ms) | S(p) | E(p) |"
    echo "|---|---:|---:|---:|---:|---:|"
    echo "| Sequencial | 1 | ${TSEQ} | $(faixa sequencial 1) | 1.00 | 1.00 |"
    for t in $THREADS; do
        tp=$(mediana paralela "$t")
        echo "| Paralela | $t | $tp | $(faixa paralela "$t") | $(awk -v a="$TSEQ" -v b="$tp" 'BEGIN{printf "%.2f", a/b}') | $(awk -v a="$TSEQ" -v b="$tp" -v p="$t" 'BEGIN{printf "%.2f", a/b/p}') |"
    done
} > "$MD"
cat "$MD"
echo
echo "Dados brutos: $CSV"
if command -v python3 >/dev/null 2>&1; then
    python3 scripts/graficos.py || echo "(graficos nao gerados: instale matplotlib com 'pip3 install matplotlib')"
fi
