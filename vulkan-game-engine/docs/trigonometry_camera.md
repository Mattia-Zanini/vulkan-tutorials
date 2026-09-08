# Trigonometria della Telecamera e Frustum di Proiezione

Per comprendere come vengono calcolati i parametri geometrici della matrice di proiezione prospettica, la chiave risiede nella scomposizione del **frustum tridimensionale** nei suoi due triangoli rettangoli bidimensionali:
* La **sezione verticale** ($Y-Z$)
* La **sezione orizzontale** ($X-Z$)

---

### 💡 Intuizione: L'Analogia della Finestra

> [!TIP]
> **Immagina di guardare attraverso una finestra rettangolare:**
> 1. **Occhio $\to$ Vetro:** Il tuo occhio si trova a distanza $n$ dal vetro. Questa distanza è il **cateto orizzontale** (adiacente).
> 2. **Centro $\to$ Bordo Superiore:** Il bordo superiore della finestra si trova a un'altezza $t$ dal centro. Questa altezza è il **cateto verticale** (opposto).
> 3. **Angolo di Visuale:** L'angolo di apertura della visuale dal centro al bordo superiore è $\frac{\theta}{2}$.
>
> In trigonometria, la funzione che mette in relazione i due cateti senza dover misurare la linea diagonale (ipotenusa) è la **tangente**:
>
> $$
> \tan\left(\frac{\theta}{2}\right) = \frac{\text{cateto opposto}}{\text{cateto adiacente}} = \frac{\text{altezza bordo}}{n}
> $$
>
> Usiamo la tangente perché conosciamo la distanza dello schermo ($n$) e vogliamo calcolare le dimensioni del rettangolo di proiezione ($t$ ed $r$), senza preoccuparci della diagonale dei raggi visivi.

---

<div align="center">

![Frustum della Telecamera e Trigonometria](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRZzMOFQ3L18u9qSaDjLoKyTc5BWhGmXo7vSexdo-B8t5stUSN13xnmTbpF&s=10)

*Rappresentazione geometrica del Frustum di visuale e parametri di proiezione*
</div>

---

## 1. Perché si usa la Tangente ($\tan$) e non Seno o Coseno?

In un triangolo rettangolo con angolo acuto $\alpha$:

$$
\sin(\alpha) = \frac{\text{cateto opposto}}{\text{ipotenusa}}, \quad \cos(\alpha) = \frac{\text{cateto adiacente}}{\text{ipotenusa}}, \quad \tan(\alpha) = \frac{\text{cateto opposto}}{\text{cateto adiacente}}
$$

```text
                      /|
          Ipotenusa  / | Cateto Opposto (Altezza: top)
    (Raggio visivo) /  |
                   /   |
        Occhio (α)/____| Cateto Adiacente (Distanza: n)
```

### Confronto delle funzioni trigonometriche

| Funzione | Rapporto | Ruolo nella Telecamera | Valutazione |
| :--- | :--- | :--- | :--- |
| $\sin(\alpha)$ | $\frac{\text{cateto opposto}}{\text{ipotenusa}}$ | Lega l'altezza del piano al raggio diagonale | ❌ Inutile: richiede di calcolare l'ipotenusa |
| $\cos(\alpha)$ | $\frac{\text{cateto adiacente}}{\text{ipotenusa}}$ | Lega la distanza $n$ al raggio diagonale | ❌ Inutile: introduce una dipendenza dall'ipotenusa |
| $\tan(\alpha)$ | $\frac{\text{cateto opposto}}{\text{cateto adiacente}}$ | Lega direttamente l'altezza ($\text{top}$) alla distanza ($n$) | ✅ **Ideale**: usa solo grandezze note o desiderate |

Nella telecamera virtuale:
* **Cosa conosciamo:** La distanza $n$ lungo l'asse ottico $Z$ (il piano *Near*). Questo è il **cateto adiacente**.
* **Cosa vogliamo ricavare:** I limiti geometrici della finestra di proiezione ($\text{top}$, $\text{bottom}$, $\text{right}$, $\text{left}$). Questi sono i **cateti opposti**.
* **Cosa NON ci interessa calcolare:** La lunghezza esatta del raggio visivo diagonale (l'ipotenusa).

> [!NOTE]
> Poiché l'ipotenusa è superflua, l'uso di $\sin$ e $\cos$ richiederebbe un passaggio aggiuntivo di normalizzazione vettoriale. La funzione **tangente** collega direttamente il dato noto ($n$) al dato ignoto ($\text{top}$).

---

## 2. Sezione Laterale ($Y-Z$): Ricavare $\text{top}$ e $\text{bottom}$

Sezionando il frustum verticalmente lungo il piano di simmetria $Y-Z$, otteniamo due triangoli rettangoli speculari:

```text
               +Y
                ▲
                │           • Bordo superiore Near (0, top, n)
                │          /|
                │         / |
                │        /  |  cateto opposto = top
                │  θ/2  /   |
   Telecamera   ├──────┼────┼────────────────────────► +Z (asse di vista)
    (0, 0, 0)   │       \   |
                │        \  |  cateto opposto = bottom (in modulo = top)
                │         \ |
                │          \|
                │           • Bordo inferiore Near (0, bottom, n)
                │      ◄────►
                │     distanza n
```

### Passaggi di calcolo:

1. L'angolo totale di apertura verticale della telecamera è il **Vertical FOV** ($\theta$).
2. L'asse di vista centrale divide l'angolo a metà: l'angolo del triangolo superiore è $\frac{\theta}{2}$.
3. Applicando la definizione di tangente al triangolo superiore:

   $$
   \tan\left(\frac{\theta}{2}\right) = \frac{\text{cateto opposto}}{\text{cateto adiacente}} = \frac{\text{top}}{n}
   $$

4. Moltiplicando entrambi i membri per $n$:

   $$
   \text{top} = n \cdot \tan\left(\frac{\theta}{2}\right)
   $$

5. Poiché la visuale è centrata simmetricamente rispetto all'origine:

   $$
   \text{bottom} = -\text{top} = -n \cdot \tan\left(\frac{\theta}{2}\right)
   $$

> [!IMPORTANT]
> **Convenzione Vulkan:** In Vulkan l'asse $+Y$ dello spazio di clip punta verso il basso. I segni possono risultare invertiti a livello di viewport o matrice, ma il valore assoluto (modulo) rimane identico.

---

## 3. Sezione Orizzontale ($X-Z$) e Aspect Ratio: Ricavare $\text{right}$ e $\text{left}$

Gli schermi adottano comunemente proporzioni panoramiche (es. 16:9, 16:10), descritte dall'**Aspect Ratio** ($a$):

$$
a = \frac{\text{larghezza}}{\text{altezza}} = \frac{2 \cdot \text{right}}{2 \cdot \text{top}} = \frac{\text{right}}{\text{top}}
$$

Da questa relazione diretta si ricavano le estensioni orizzontali:

$$
\text{right} = a \cdot \text{top} = a \cdot n \cdot \tan\left(\frac{\theta}{2}\right)
$$

$$
\text{left} = -\text{right} = -a \cdot n \cdot \tan\left(\frac{\theta}{2}\right)
$$

---

## 4. Perché nella Matrice di Proiezione compare $1 / \tan(\theta/2)$?

Un risultato fondamentale dell'algebra della grafica 3D è che **la distanza $n$ scompare dalle prime due righe della matrice**.

Per mappare un punto generico $(x, y, z)$ nelle coordinate normalizzate di dispositivo (NDC) $[-1, 1]$:

1. **Proiezione sul piano *Near*** per similitudine tra triangoli:

   $$
   x_s = n \cdot \frac{x}{z}, \quad y_s = n \cdot \frac{y}{z}
   $$

2. **Normalizzazione della coordinata orizzontale** dividendo per la semi-larghezza $\text{right}$:

   $$
   x_{\text{NDC}} = \frac{x_s}{\text{right}} = \frac{n \cdot \frac{x}{z}}{a \cdot n \cdot \tan(\theta/2)}
   $$

3. **Semplificazione di $n$**: Il parametro $n$ compare sia a numeratore che a denominatore, **semplificandosi**:

   $$
   x_{\text{NDC}} = \frac{x}{z} \cdot \left( \frac{1}{a \cdot \tan(\theta/2)} \right)
   $$

4. **Normalizzazione della coordinata verticale**:

   $$
   y_{\text{NDC}} = \frac{y_s}{\text{top}} = \frac{n \cdot \frac{y}{z}}{n \cdot \tan(\theta/2)} = \frac{y}{z} \cdot \left( \frac{1}{\tan(\theta/2)} \right)
   $$

I coefficienti diagonali della matrice di proiezione prospettica $P$ sono proprio questi fattori moltiplicativi:

$$
P_{00} = \frac{1}{a \cdot \tan(\theta/2)}, \quad P_{11} = \frac{1}{\tan(\theta/2)}
$$

---

## 5. Come viene Deformato un Cubo nel Frustum

Consideriamo un cubo di lato $L$ posizionato all'interno del frustum e allineato lungo l'asse ottico $Z$:
* La faccia anteriore si trova a distanza $z_1$.
* La faccia posteriore si trova a distanza $z_2$ (con $z_2 > z_1$).

```text
            FRUSTUM (Spazio Vista 3D)                  VOLUME NDC (Spazio Canonico GPU)
                 Z_near       Z_far
                   │            │                                  -1      +1
    Telecamera     │ ┌────────┐ │                                   ┌──────┐
        • ─────────┼─┤        ├─┼───────►                      -1 ──┤ Cubo ├── +1
                   │ └────────┘ │                                   │deform│
                   │ Faccia   Faccia                                └──────┘
                   │ vicina   lontana
                   │ (grande) (piccola)
```

1. **Nello Spazio Vista:** Il cubo possiede lati dimensionalmente uguali, ma la faccia posteriore (più lontana) sottende un angolo visivo inferiore rispetto alla faccia anteriore.
2. **Durante la Divisione Prospettica:** Le coordinate vengono scalate per la rispettiva distanza $z$:

   $$
   \text{Larghezza proiettata (fronte)} = \frac{n \cdot L}{z_1}
   $$

   $$
   \text{Larghezza proiettata (retro)} = \frac{n \cdot L}{z_2}
   $$

   Dato che $z_2 > z_1$, la larghezza proiettata della faccia posteriore è minore di quella anteriore.
3. **Nel Volume NDC $[-1, 1]^3$:** La trasformazione di proiezione rimappa il volume tronco-piramidale del frustum in un parallelepipedo regolare. All'interno di questo spazio normalizzato, il cubo assume la forma di un **tronco di piramide inverso**: le sue linee parallele convergono verso il fondo, ricreando la corretta prospettiva centrale.

---

Il diagramma interattivo sottostante permette di osservare sia la vista prospettica 3D del cubo nel frustum, sia la sezione bidimensionale con i triangoli rettangoli e le formule trigonometriche corrispondenti: