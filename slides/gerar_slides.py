"""Gera slides/apresentacao.pdf (16:9). Le results/ se existir (rode `make bench` antes).
Requer: pip3 install reportlab"""
import os, csv, statistics
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, white, black

R = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
W, H = 960, 540
AZ, AZC, LAR, CINZA, TX = HexColor("#1F3864"), HexColor("#4A90D9"), HexColor("#E8892B"), HexColor("#F2F4F8"), HexColor("#222222")
c = canvas.Canvas(os.path.join(R, "slides", "apresentacao.pdf"), pagesize=(W, H))
c.setTitle("Contagem paralela de objetos em matriz binaria"); c.setAuthor("Lucas Flor")
n = [0]

def novo(titulo, sub=None):
    if n[0]: c.showPage()
    n[0] += 1
    c.setFillColor(white); c.rect(0, 0, W, H, fill=1, stroke=0)
    c.setFillColor(AZ); c.setFont("Helvetica-Bold", 28); c.drawString(50, H - 65, titulo)
    c.setFillColor(LAR); c.rect(50, H - 80, 70, 4, fill=1, stroke=0)
    if sub:
        c.setFillColor(HexColor("#555555")); c.setFont("Helvetica", 14); c.drawString(50, H - 102, sub)
    c.setFont("Helvetica", 9); c.setFillColor(HexColor("#888888"))
    c.drawString(50, 20, "Sistemas Operacionais 2026/II - PUCRS - Lucas Flor"); c.drawRightString(W - 50, 20, str(n[0]))

def bullets(itens, x=60, y=H - 140, tam=17, larg=840, gap=10):
    c.setFillColor(TX)
    for it in itens:
        sub = it.startswith("  ")
        f = tam - 3 if sub else tam
        c.setFont("Helvetica", f)
        pal, linha, linhas = it.strip().split(), "", []
        for p in pal:
            t = (linha + " " + p).strip()
            if c.stringWidth(t, "Helvetica", f) > larg - (30 if sub else 0):
                linhas.append(linha); linha = p
            else: linha = t
        linhas.append(linha)
        for i, l in enumerate(linhas):
            if i == 0:
                c.setFillColor(LAR if not sub else AZC); c.drawString(x + (25 if sub else 0), y, "-" if sub else "\u2022"); c.setFillColor(TX)
            c.drawString(x + (40 if sub else 18), y, l); y -= f + 5
        y -= gap
    return y

def grade(M, x, y, cel, blocos=None, rot=None):
    L, C = len(M), len(M[0])
    for i in range(L):
        for j in range(C):
            c.setFillColor(AZC if M[i][j] else CINZA); c.setStrokeColor(white)
            c.rect(x + j * cel, y - (i + 1) * cel, cel, cel, fill=1, stroke=1)
            if rot and rot[i][j]:
                c.setFillColor(white); c.setFont("Helvetica-Bold", cel * 0.5)
                c.drawCentredString(x + j * cel + cel / 2, y - (i + 1) * cel + cel * 0.32, rot[i][j])
    if blocos:
        c.setStrokeColor(LAR); c.setLineWidth(4)
        bl, bc = blocos
        for k in range(1, bl): yy = y - (k * L // bl) * cel; c.line(x, yy, x + C * cel, yy)
        for k in range(1, bc): xx = x + (k * C // bc) * cel; c.line(xx, y, xx, y - L * cel)
        c.setLineWidth(1)

def codigo(linhas, x, y, tam=13, larg=420):
    c.setFillColor(HexColor("#1E1E1E")); c.roundRect(x - 10, y - len(linhas) * (tam + 4) - 8, larg, len(linhas) * (tam + 4) + 22, 6, fill=1, stroke=0)
    c.setFont("Courier", tam); c.setFillColor(HexColor("#D4D4D4"))
    for l in linhas: c.drawString(x, y, l); y -= tam + 4

def ler(nome):
    with open(os.path.join(R, "tests", "obrigatorios", nome)) as f:
        v = f.read().split(); l, cc = int(v[0]), int(v[1])
        return [[int(v[2 + i * cc + j]) for j in range(cc)] for i in range(l)]

EX1, EX3 = ler("ex1_5x5.txt"), ler("ex3_8x8.txt")

# 1 - Capa
c.setFillColor(AZ); c.rect(0, 0, W, H, fill=1, stroke=0); n[0] = 1
c.setFillColor(white); c.setFont("Helvetica-Bold", 38); c.drawString(60, 330, "Contagem paralela de objetos")
c.drawString(60, 285, "em uma matriz binaria")
c.setFillColor(LAR); c.rect(60, 262, 90, 5, fill=1, stroke=0)
c.setFillColor(white); c.setFont("Helvetica", 18)
c.drawString(60, 225, "Versao sequencial (flood fill) e paralela com POSIX Threads")
c.setFont("Helvetica", 15); c.drawString(60, 150, "Lucas Flor")
c.drawString(60, 128, "Sistemas Operacionais 2026/II - Prof. Filipo Novo Mor - PUCRS")
grade(EX3, 700, 400, 26, (2, 2))

# 2 - Problema
novo("O problema", "Contar componentes de 1s com conectividade 8")
bullets(["Matriz binaria: 0 = fundo, 1 = primeiro plano",
         "Objeto = celulas 1 ligadas por lado OU canto (8 vizinhos)",
         "Exemplo 1 (5x5): 3 objetos",
         "Desafio paralelo: um objeto pode atravessar as divisoes da matriz - somar contagens locais NAO basta"], larg=480)
grade(EX1, 600, 420, 48)
c.setFillColor(TX); c.setFont("Helvetica", 12); c.drawString(600, 165, "8 vizinhos: (-1,-1) (-1,0) (-1,1) (0,-1) (0,1) (1,-1) (1,0) (1,1)")

# 3 - Estrategia
novo("Estrategia escolhida", "Uma referencia sequencial + uma versao paralela equivalente")
bullets(["Sequencial: flood fill ITERATIVO (pilha explicita) - referencia de correcao e de tempo",
         "Paralela: Pthreads (memoria compartilhada, sem copiar a matriz)",
         "  Matriz dividida em grade BL x BC de blocos (configuravel)",
         "  Fila dinamica de blocos: threads pegam o proximo bloco livre",
         "  Fase 1 (paralela): rotular componentes locais de cada bloco",
         "  Fase 2 (paralela): verificar fronteiras e unir rotulos (Union-Find)",
         "Objetos = soma das contagens locais - unioes efetivas",
         "ANSI C89, -Wall -Wextra -pedantic sem avisos, Linux e macOS"])

# 4 - Sequencial
novo("Versao sequencial", "conta-objetos-sequencial.c")
codigo(["para cada celula i (ordem de linhas):",
        "  se M[i]==1 e nao visitado[i]:",
        "    objetos++",
        "    visitado[i]=1; empilha(i)",
        "    enquanto pilha nao vazia:",
        "      cel = desempilha()",
        "      para cada um dos 8 vizinhos v:",
        "        se M[v]==1 e nao visitado[v]:",
        "          visitado[v]=1; empilha(v)"], 70, 400, 14, 440)
bullets(["Tempo O(L x C): cada celula entra na pilha no maximo 1 vez",
         "Marca ao EMPILHAR: sem duplicatas",
         "Sem recursao: pilha no heap cresce com realloc (recursao estouraria a stack em objetos grandes)",
         "Resultado das 5 matrizes = valores esperados"], x=540, y=390, tam=15, larg=380)

# 5 - Decomposicao
novo("Decomposicao e fila dinamica", "./conta-objetos-paralelo arquivo <threads> [BL BC]")
grade(EX3, 70, 420, 34, (2, 2))
bullets(["Bloco (i,j): linhas [i*L/BL, (i+1)*L/BL) - sobras distribuidas",
         "Reproduz as grades 2x2 e 3x3 do enunciado",
         "Mais blocos que threads: fila dinamica balanceia a carga",
         "Fila = indice 'proximo' protegido por mtx_fila",
         "Cada celula pertence a UM bloco: escrita em rotulo[] sem lock"], x=400, y=400, tam=16, larg=500)

# 6 - Fase 1
novo("Fase 1 - rotulos locais (paralela)", "Flood fill restrito aos limites do bloco")
rot = [["" for _ in range(8)] for _ in range(8)]
seeds = {"A": [(0, 0), (0, 1), (1, 0)], "B": [(3, 3)], "C": [(3, 4)], "F": [(2, 6), (3, 6)], "D": [(4, 3)], "E": [(4, 4)], "G": [(6, 2), (7, 2)], "H": [(6, 7), (7, 6), (7, 7)]}
for k, v in seeds.items():
    for (i, j) in v: rot[i][j] = k
grade(EX3, 70, 420, 34, (2, 2), rot)
bullets(["Rotulo = indice da celula-semente + 1 (l*C + c + 1)",
         "Unico na matriz inteira sem contador global nem lock",
         "Cada thread tem a sua propria pilha",
         "Exemplo 3: 8 componentes locais (A..H)",
         "Objeto central B,C,D,E aparece 4 vezes!",
         "pthread_join = barreira antes da fase 2 (macOS nao tem pthread_barrier)"], x=400, y=400, tam=16, larg=500)

# 7 - Sincronizacao
novo("Sincronizacao e regioes criticas")
lin = [("Recurso", "Risco", "Protecao"),
       ("proximo (fila)", "2 threads pegam o mesmo bloco", "mtx_fila"),
       ("pai[] + unioes", "corrida no find/union, uniao perdida", "mtx_uf"),
       ("rotulo[]", "escrita simultanea", "blocos disjuntos + join"),
       ("matriz", "-", "somente leitura"),
       ("fase1 -> fase2", "ler rotulos incompletos", "pthread_join")]
y = 410
for i, r in enumerate(lin):
    c.setFillColor(AZ if i == 0 else (CINZA if i % 2 else white)); c.rect(60, y - 10, 840, 34, fill=1, stroke=0)
    c.setFillColor(white if i == 0 else TX); c.setFont("Helvetica-Bold" if i == 0 else "Helvetica", 15)
    c.drawString(75, y, r[0]); c.drawString(300, y, r[1]); c.drawString(660, y, r[2]); y -= 36
bullets(["Sem deadlock: nenhuma thread segura 2 mutexes ao mesmo tempo (sem espera circular)",
         "ThreadSanitizer: nenhuma data race reportada"], y=170, tam=16)

# 8 - Fase 2
novo("Fase 2 - fronteiras e consolidacao", "Cada bloco verifica a sua borda inferior e a direita")
bullets(["Borda inferior: (r1-1, c) x (r1, c-1), (r1, c), (r1, c+1)",
         "Borda direita: (l, c1-1) x (l-1, c1), (l, c1), (l+1, c1)",
         "Reta + 2 diagonais = horizontal, vertical e diagonal",
         "Diagonais que cruzam a linha/coluna de corte tratam o ENCONTRO DE 4 BLOCOS",
         "Union-Find com compressao de caminho; menor rotulo vira raiz (deterministico)",
         "unir(): se raizes diferentes -> une e unioes++ (dentro de mtx_uf)",
         "Verificar o mesmo par 2 vezes nao faz mal: so conta uniao efetiva"])

# 9 - Exemplo rastreavel
novo("Exemplo rastreavel - Exemplo 3 (grade 2x2)")
grade(EX3, 70, 430, 36, (2, 2), rot)
c.setFillColor(TX); c.setFont("Helvetica", 18)
for i, t in enumerate(["Componentes locais: A B | C F | D G | E H",
                       "Soma local = 8",
                       "Unioes efetivas: B-C, B-D, B-E = 3",
                       "(C-D, C-E, D-E: ja na mesma classe)",
                       "Objetos = 8 - 3 = 5   (esperado: 5)"]):
    c.setFont("Helvetica-Bold" if i == 4 else "Helvetica", 18); c.drawString(430, 390 - i * 42, t)
c.setFont("Courier", 12); c.drawString(430, 150, "$ ./conta-objetos-paralelo ex3_8x8.txt 4 2 2 -v")
c.drawString(430, 132, "  Soma local: 8 | Unioes nas fronteiras: 3")
c.drawString(430, 114, "Objetos: 5")

# 10 - Demonstracao
novo("Demonstracao", "Terminal")
codigo(["$ make",
        "$ ./conta-objetos-sequencial tests/obrigatorios/ex5_12x12.txt",
        "$ ./conta-objetos-paralelo tests/obrigatorios/ex5_12x12.txt 9 3 3 -v",
        "$ ./conta-objetos-paralelo tests/obrigatorios/ex3_8x8.txt 4 2 2 -v",
        "$ make test",
        "$ cat results/resumo_desempenho.md"], 80, 400, 16, 800)

# 11 - Testes
novo("Testes funcionais", "make test - 8 configuracoes paralelas x 5 repeticoes por matriz")
lin = [("Ex.", "Dim.", "Esperado", "Sequencial", "Paralelo")] + [
    (str(i + 1), d, e, e, e) for i, (d, e) in enumerate([("5x5", "3"), ("6x8", "4"), ("8x8", "5"), ("9x12", "6"), ("12x12", "7")])]
y = 410
for i, r in enumerate(lin):
    c.setFillColor(AZ if i == 0 else (CINZA if i % 2 else white)); c.rect(60, y - 10, 470, 32, fill=1, stroke=0)
    c.setFillColor(white if i == 0 else TX); c.setFont("Helvetica-Bold" if i == 0 else "Helvetica", 15)
    for k, x in enumerate([75, 130, 220, 320, 430]): c.drawString(x, y, r[k])
    y -= 34
bullets(["Adicionais: zeros (0), tudo 1 (1), xadrez so diagonal (1), X no encontro de 4 blocos (1), serpente (1), 25 pontos isolados (25), 1 linha (7)",
         "Configs: 1:1x1, 2:2x1, 2:1x2, 4:2x2, 4:3x3, 3:3x3, 8:4x4, 2:6x6",
         "40 matrizes aleatorias + ThreadSanitizer: tudo identico"], x=560, y=400, tam=14, larg=360)

# 12 - Desempenho
novo("Desempenho", "Matriz 4000x4000, 45% de 1s, mediana de 5 repeticoes (make bench)")
csvp = os.path.join(R, "results", "medicoes.csv")
if os.path.exists(csvp):
    d = {}
    for l in csv.DictReader(open(csvp)):
        d.setdefault((l["versao"], int(l["trabalhadores"])), []).append(float(l["tempo_ms"]))
    ts = statistics.median(d[("sequencial", 1)])
    lin = [("Versao", "p", "T (ms)", "S(p)", "E(p)"), ("Sequencial", "1", "%.1f" % ts, "1.00", "1.00")]
    for p in sorted(p for v, p in d if v == "paralela"):
        t = statistics.median(d[("paralela", p)]); lin.append(("Paralela", str(p), "%.1f" % t, "%.2f" % (ts / t), "%.2f" % (ts / t / p)))
    y = 410
    for i, r in enumerate(lin):
        c.setFillColor(AZ if i == 0 else (CINZA if i % 2 else white)); c.rect(60, y - 10, 420, 32, fill=1, stroke=0)
        c.setFillColor(white if i == 0 else TX); c.setFont("Helvetica-Bold" if i == 0 else "Helvetica", 15)
        for k, x in enumerate([75, 190, 240, 340, 410]): c.drawString(x, y, r[k])
        y -= 34
    g = os.path.join(R, "results", "grafico-aceleracao.png")
    if os.path.exists(g): c.drawImage(g, 500, 90, width=420, height=270, preserveAspectRatio=True)
else:
    bullets(["Execute 'make bench' e depois 'python3 slides/gerar_slides.py' para preencher este slide com os seus numeros."])

# 13 - Analise
novo("Analise dos resultados")
bullets(["Matrizes pequenas: paralelo mais lento - criar threads custa mais que o trabalho",
         "Paralelo com 1 thread < sequencial: rotulo[] e pai[] usam int (4 B) contra 1 B do visitado -> mais trafego de memoria; 2 fases + criacao de threads",
         "Ganho abaixo do ideal (Amdahl): leitura, joins e soma final sao sequenciais; algoritmo limitado por largura de banda de memoria",
         "Apple Silicon: nucleos P e E - acima do numero de nucleos P a eficiencia cai",
         "Consolidacao barata: O(perimetro dos blocos) << O(L x C); pouca disputa em mtx_uf"])

# 14 - Conclusoes
novo("Conclusoes")
bullets(["Sequencial e paralelo sempre iguais: 5 obrigatorias, 7 adicionais, 8 configuracoes, 5 repeticoes",
         "Chave da correcao: rotulos globalmente unicos + Union-Find com regiao critica minima",
         "Paralelismo efetivo: o flood fill (parte cara) roda simultaneamente nas threads",
         "Aprendizado: paralelizar = dividir dados + identificar estado compartilhado + projetar a consolidacao",
         "Melhoria futura: equivalencias locais por thread e reducao sem lock"])
c.save()
print("slides/apresentacao.pdf gerado (%d slides)" % n[0])
