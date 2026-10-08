"""Gera graficos de tempo, aceleracao e eficiencia a partir de results/medicoes.csv."""
import csv, statistics, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

base = os.path.join(os.path.dirname(__file__), "..", "results")
dados = {}
with open(os.path.join(base, "medicoes.csv")) as f:
    for l in csv.DictReader(f):
        dados.setdefault((l["versao"], int(l["trabalhadores"])), []).append(float(l["tempo_ms"]))
tseq = statistics.median(dados[("sequencial", 1)])
ps = sorted(p for v, p in dados if v == "paralela")
tp = [statistics.median(dados[("paralela", p)]) for p in ps]
s = [tseq / t for t in tp]
e = [x / p for x, p in zip(s, ps)]

def salvar(nome, titulo, ylabel, y, ideal=None, ref=None):
    plt.figure(figsize=(7, 4.5))
    plt.plot(ps, y, "o-", label="Paralela (pthreads)")
    if ideal: plt.plot(ps, ps, "--", color="gray", label="Ideal S(p)=p")
    if ref is not None: plt.axhline(ref, color="red", ls=":", label="Sequencial")
    plt.title(titulo); plt.xlabel("Threads (p)"); plt.ylabel(ylabel)
    plt.xticks(ps); plt.ylim(bottom=0); plt.grid(alpha=.3); plt.legend()
    plt.tight_layout(); plt.savefig(os.path.join(base, nome), dpi=120); plt.close()

salvar("grafico-tempo.png", "Tempo de execucao (mediana)", "Tempo (ms)", tp, ref=tseq)
salvar("grafico-aceleracao.png", "Aceleracao S(p) = Tseq / Tpar", "S(p)", s, ideal=True)
salvar("grafico-eficiencia.png", "Eficiencia E(p) = S(p) / p", "E(p)", e)
print("Graficos gerados em results/")
