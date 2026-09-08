Il video [The Math behind (most) 3D games - Perspective Projection](https://www.youtube.com/watch?v=U0_ONQQ5ZNM&utm_source=gemini) di Brendan Galea affronta uno dei pilastri della computer grafica moderna: **come trasformare una scena geometrica tridimensionale in un'immagine bidimensionale** conservando il senso di profondità e rispettando le convenzioni hardware dell'API Vulkan.

Di seguito trovi la trattazione esaustiva e rigorosa di tutti gli argomenti, i modelli matematici e gli esempi numerici presentati nel video, affiancati da brevi spiegazioni intuitive di supporto.

---

## 1. La Sfida della Visione: Ray Tracing vs. Rasterizzazione

Il video apre analizzando il funzionamento della vista umana e dei display digitali:

* La retina dell'occhio e gli schermi dei computer sono **superfici piane bidimensionali**. Il cervello ricostruisce la terza dimensione interpretando indizi visivi quali la scala prospettica, l'occlusione e la parallasse.
* Pensando allo schermo come a una finestra trasparente sul mondo virtuale, esistono due approcci duali per determinare il colore dei pixel:

```
           APPROCCIO 1: IMAGE-ORDER (Ray Tracing)
  Telecamera ──[ Raggio attraverso il pixel ]──► Scena (Trova collisione)

           APPROCCIO 2: OBJECT-ORDER (Rasterizzazione)
  Oggetto 3D ──[ Proiezione matriciale ]──────► Schermo (Disegna sui pixel)

```

| Proprietà | Image-Order Rendering (Ray Tracing) | Object-Order Rendering (Rasterizzazione) |
| --- | --- | --- |
| **Punto di partenza** | Si itera **pixel per pixel** sullo schermo. | Si itera **oggetto per oggetto** (triangolo per triangolo). |
| **Meccanismo** | Spara uno o più raggi dalla telecamera attraverso ogni pixel per calcolare le intersezioni. | I vertici 3D vengono proiettati matematicamente sul piano dello schermo tramite matrici $4 \times 4$. |
| **Costo computazionale** | Storicamente molto lento su CPU (ad es. 60–160 ore per singolo frame in film d'animazione come *Toy Story 4*). | Estremamente veloce ed efficiente: algoritmicamente affine all'architettura parallela (SIMD/SIMT) delle GPU. |

> **In parole semplici:** Nel Ray Tracing guardi ogni buco della zanzariera (pixel) e chiedi: *"Quale raggio di luce ci passa attraverso?"*. Nella rasterizzazione prendi ciascun triangolo 3D e usi una formula matematica per spiaccicarlo direttamente sul vetro della finestra.

---

## 2. Il Volume di Vista Canonico di Vulkan e la Proiezione Ortografica

Prima di affrontare la prospettiva, il video definisce il sistema di coordinate di destinazione della GPU: lo **Spazio Canonico di Vista** (*Canonical Viewing Volume* o coordinate NDC - *Normalized Device Coordinates*).

---

<div align="center">

![Volume di Vista Canonico NDC in Vulkan](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcQCaVQM7p-70ZjgN4EnMaH6gLkhBzfi_YHRskOkD3UOKv9tU8xs7wkgTmz_&s=10)

</div>

In Vulkan, la GPU visualizza a schermo unicamente ciò che ricade all'interno di questo parallelepipedo:

* $X_{\text{NDC}} \in [-1, 1]$ (da sinistra verso destra)
* $Y_{\text{NDC}} \in [-1, 1]$ (**con l'asse $+Y$ che punta verso il basso**, contrariamente ad OpenGL)
* $Z_{\text{NDC}} \in [0, 1]$ (**con l'asse $+Z$ che entra nello schermo**, intervallo $[0, 1]$ anziché $[-1, 1]$ di OpenGL)

### Il volume ortografico e la sua matrice

Un volume di vista ortografico (*Orthographic View Volume*) è un parallelepipedo allineato agli assi, delimitato da 6 piani di taglio (*clipping planes*):

* Sinistra ($l$) e Destra ($r$) su $X$
* Alto ($t$) e Basso ($b$) su $Y$
* Vicino ($n$, Near) e Lontano ($f$, Far) su $Z$

Per mappare questo box arbitrario nel volume canonico di Vulkan $[-1, 1] \times [-1, 1] \times [0, 1]$, si combinano due trasformazioni elementari:

1. **Traslazione:** Sposta il centro del piano near all'origine.
2. **Scalatura:** Ridimensiona le estensioni del box affinché combacino con le dimensioni di Vulkan:
* Su $X$: ampiezza $r - l \longrightarrow \text{ampiezza canonica } 2 \implies s_x = \frac{2}{r - l}$
* Su $Y$: ampiezza $b - t \longrightarrow \text{ampiezza canonica } 2 \implies s_y = \frac{2}{b - t}$
* Su $Z$: ampiezza $f - n \longrightarrow \text{ampiezza canonica } 1 \implies s_z = \frac{1}{f - n}$



> **In parole semplici:** La proiezione ortografica è come una fotocopia in scala: non c'è profondità, gli oggetti lontani rimangono grandi esattamente quanto quelli vicini. La matrice serve solo a prendere una scatola 3D di qualsiasi grandezza e scalarla dentro la scatola standard della scheda video.

---

## 3. La Proiezione Prospettica e il Trucco delle Coordinate Omogenee

Nella realtà gli oggetti più distanti appaiono più piccoli. Lo spazio visivo della telecamera non è un parallelepipedo, ma una piramide tronca: il **Frustum di vista**.

### Teorema dei Triangoli Simili

Osservando il piano di proiezione (posto a distanza $z = n$ dall'occhio) e un punto nello spazio $P = (x, y, z)$:

```
           Y
           ▲
           │          • P(x, y, z)
           │         /│
       ys  │•       / │ y
           ││\     /  │
           ││ \   /   │
       ────┴┴──╲─/────┴────► Z
          Eye   n     z

```

Per la proporzione tra triangoli simili:


$$
\frac{y_s}{n} = \frac{y}{z} \implies y_s = n \frac{y}{z}
$$

$$
\frac{x_s}{n} = \frac{x}{z} \implies x_s = n \frac{x}{z}
$$

### Il blocco algebrico delle matrici lineari

Una moltiplicazione matrice-vettore standard produce esclusivamente **combinazioni lineari**:


$$
x' = a_{11}x + a_{12}y + a_{13}z + a_{14}
$$


È **algebricamente impossibile** calcolare una divisione per una variabile (come $\frac{x}{z}$ o $\frac{y}{z}$) usando solo coefficienti costanti in una matrice.

### La soluzione: Coordinate Omogenee e Divisione Prospettica Hardware

Nello spazio proiettivo, un punto in coordinate omogenee $[x, y, z, w]^T$ corrisponde alle coordinate cartesiane tridimensionali:


$$
\left( \frac{x}{w}, \; \frac{y}{w}, \; \frac{z}{w} \right)
$$

Nei chip grafici moderni, il passaggio di divisione per $w$ (**Perspective Divide**) è cablato a livello hardware e viene eseguito automaticamente subito dopo il vertex shader sull'output `gl_Position`:

$$
\text{gl_Position} = \begin{bmatrix} x_{\text{clip}} \\ y_{\text{clip}} \\ z_{\text{clip}} \\ w_{\text{clip}} \end{bmatrix} \xrightarrow{\text{Hardware}} \mathbf{v}_{\text{NDC}} = \begin{bmatrix} x_{\text{clip}} / w_{\text{clip}} \\ y_{\text{clip}} / w_{\text{clip}} \\ z_{\text{clip}} / w_{\text{clip}} \end{bmatrix}
$$

Se progettiamo la matrice affinché la quarta riga imposti:


$$
w_{\text{clip}} = z
$$


la GPU dividerà automaticamente tutte le altre componenti per la profondità $z$ del punto.

> **In parole semplici:** Le matrici sanno solo sommare e moltiplicare, non possono dividere per la distanza $z$. Ma la scheda video possiede una funzione automatica che divide sempre le coordinate per il quarto valore ($w$). Inserendo la profondità $z$ dentro $w$, la GPU eseguirà la divisione prospettica per noi.

---

## 4. La Gestione della Coordinata Z: Il Depth Buffer Non Lineare

Costruendo la matrice di deformazione prospettica $M_{\text{persp}}$:

* Righe 1 e 2 moltiplicano per $n$: producono $n \cdot x$ e $n \cdot y$, che divisi per $w' = z$ daranno $n \cdot \frac{x}{z}$ e $n \cdot \frac{y}{z}$.
* Riga 4 è impostata a $\begin{bmatrix} 0 & 0 & 1 & 0 \end{bmatrix}$: copia $z$ dentro $w'$.

Cosa inserire nella terza riga per gestire la coordinata $Z$?

* **L'errore comune:** Usare $\begin{bmatrix} 0 & 0 & 1 & 0 \end{bmatrix}$. Dopo la divisione prospettica otterremmo:

$$
z_{\text{proj}} = \frac{z'}{w'} = \frac{z}{z} = 1
$$



Tutti i vertici della scena finirebbero appiattiti su $z=1$, cancellando ogni informazione di profondità e rendendo impossibile il test di occlusione (Depth Test).
* **Il vincolo quadratico:** Per avere $z_{\text{proj}} = z$ servirebbe $z' = z^2$. Ma una riga di matrice calcola al più $m_1 z + m_2$:

$$
m_1 z + m_2 = z^2
$$



Un'equazione di secondo grado può avere al massimo **due soluzioni reali**.

### La derivazione dei coefficienti $m_1$ ed $m_2$

Si impone che la relazione sia esatta almeno sui due piani di confine, $z = n$ (near) e $z = f$ (far):


$$
\begin{cases} m_1 n + m_2 = n^2 \\ m_1 f + m_2 = f^2 \end{cases}
$$

Sottraendo la prima equazione dalla seconda:


$$
m_1(f - n) = f^2 - n^2 = (f - n)(f + n) \implies m_1 = f + n
$$


Sostituendo $m_1$ nella prima equazione:


$$
(f + n)n + m_2 = n^2 \implies fn + n^2 + m_2 = n^2 \implies m_2 = -fn
$$

La terza componente prima della divisione risulta:


$$
z' = (f + n)z - fn
$$

E dopo la divisione prospettica per $w'=z$:


$$
z_{\text{proj}} = \frac{z'}{w'} = \frac{(f + n)z - fn}{z} = (f + n) - \frac{fn}{z}
$$

```
  z_proj
   1.0 ┼───────────────────────────────······ far (z = f)
       │                         . · '
       │                    . · '
       │               . · '
       │          . · '
       │      . ·
   0.0 ┼──. ·'────────────────────────────────► z (profondità reale)
      near (z = n)

```

### Conseguenze pratiche: Z-Fighting e Precisione

1. **Monotonia preservata:** La derivata $\frac{d(z_{\text{proj}})}{dz} = \frac{fn}{z^2} > 0$ è strettamente positiva per $z \in [n, f]$. L'ordinamento degli oggetti non cambia: chi è davanti resta davanti.
2. **Distribuzione non lineare:** La curva è un'iperbole legata a $-\frac{1}{z}$. La stragrande maggioranza dei valori di precisione a virgola mobile del Depth Buffer viene concentrata nelle vicinanze del piano near.
3. **Z-Fighting:** Se due superfici sono molto vicine a grande distanza ($z \approx f$), i loro valori di profondità proiettata risulteranno quasi identici a causa dell'arrotondamento dei float, facendole tremolare visivamente a schermo. Per ridurre il fenomeno occorre mantenere il piano near il più lontano possibile dall'occhio e limitare la distanza del far plane.

> **In parole semplici:** La profondità registrata non cresce a passo costante. La scheda video dedica tantissima precisione ai dettagli a pochi centimetri dal nostro naso, e pochissima precisione agli oggetti all'orizzonte. Avvicinare troppo il piano Near all'occhio "ruba" tutta la precisione al resto del mondo 3D, causando artefatti sui poligoni lontani (Z-fighting).

---

## 5. La Matrice di Proiezione Prospettica Completa

Brendan combina la matrice prospettica $M_{\text{persp}}$ (che trasforma il frustum in un box ortografico) con la matrice ortografica $M_{\text{ortho}}$ per Vulkan:

$$
M_{\text{proj}} = M_{\text{ortho}} \cdot M_{\text{persp}}
$$

### Semplificazione: Frustum Simmetrico, FOV e Aspect Ratio

Nei videogiochi la telecamera guarda dritto attraverso il centro dello schermo, rendendo il frustum simmetrico:


$$
l = -r, \quad t = -b
$$

Invece di assegnare manualmente le coordinate geometriche $(l, r, t, b)$, si parametrizza la visuale tramite:

1. **Vertical Field of View ($\theta$ o $\text{fov}_y$):** L'angolo di apertura verticale della telecamera.
2. **Aspect Ratio ($a = \frac{\text{larghezza}}{\text{altezza}}$):** Il rapporto di forma dello schermo (es. 16:9).

Applicando la trigonometria elementare sul triangolo rettangolo del piano near:


$$
\tan\left(\frac{\theta}{2}\right) = \frac{b}{n} \implies b = n \tan\left(\frac{\theta}{2}\right) \iff \frac{n}{b} = \frac{1}{\tan(\theta / 2)}
$$

$$
r = a \cdot b = a \cdot n \tan\left(\frac{\theta}{2}\right) \iff \frac{n}{r} = \frac{1}{a \tan(\theta / 2)}
$$

### La Matrice di Proiezione Vulkan $4 \times 4$

Inserendo questi parametri e ricordando la convenzione di Vulkan ($Y$ verso il basso, $Z \in [0, 1]$), la matrice di proiezione prospettica assume la forma:

$$
P = \begin{bmatrix} \frac{1}{a \tan(\theta/2)} & 0 & 0 & 0 \\ 0 & \frac{1}{\tan(\theta/2)} & 0 & 0 \\ 0 & 0 & \frac{f}{f - n} & -\frac{f \cdot n}{f - n} \\ 0 & 0 & 1 & 0 \end{bmatrix}
$$

*(Nota: Se nel vertex shader si desidera ribaltare il segno dell'asse $Y$ per allinearlo ai sistemi up-positive, l'elemento $(1,1)$ viene negato oppure si imposta un'altezza del Viewport negativa in Vulkan 1.1+).*

---

## 6. Esempio Numerico Passo-Passo

Verifichiamo la trasformazione completa su un punto nello spazio della telecamera:

* **Parametri della telecamera:**
  * $\theta = 90^\circ \implies \theta/2 = 45^\circ \implies \tan(45^\circ) = 1$
  * Aspect Ratio: $a = 1.0$ (schermo quadrato)
  * Near plane: $n = 1.0$
  * Far plane: $f = 10.0$


* **Punto 3D nello spazio vista (Eye Space):**

$$
P_{\text{view}} = \begin{bmatrix} x \\ y \\ z \\ 1 \end{bmatrix} = \begin{bmatrix} 2.0 \\ 1.0 \\ 4.0 \\ 1.0 \end{bmatrix}
$$



*(Il punto si trova a una distanza di $z = 4.0$ unità davanti alla camera)*.

### Passo 1: Calcolo dei coefficienti della matrice $P$

* Termine $X$: $\frac{1}{a \tan(45^\circ)} = \frac{1}{1.0 \cdot 1.0} = 1.0$
* Termine $Y$: $\frac{1}{\tan(45^\circ)} = 1.0$
* Termine $Z_{33}$: $\frac{f}{f - n} = \frac{10}{10 - 1} = \frac{10}{9} \approx 1.111$
* Termine $Z_{34}$: $-\frac{f \cdot n}{f - n} = -\frac{10 \cdot 1}{9} = -\frac{10}{9} \approx -1.111$

$$
P = \begin{bmatrix} 1.0 & 0 & 0 & 0 \\ 0 & 1.0 & 0 & 0 \\ 0 & 0 & \frac{10}{9} & -\frac{10}{9} \\ 0 & 0 & 1.0 & 0 \end{bmatrix}
$$

### Passo 2: Moltiplicazione matrice-vettore (Clip Space)

$$
\mathbf{v}_{\text{clip}} = P \cdot P_{\text{view}} = \begin{bmatrix} 1.0 & 0 & 0 & 0 \\ 0 & 1.0 & 0 & 0 \\ 0 & 0 & \frac{10}{9} & -\frac{10}{9} \\ 0 & 0 & 1.0 & 0 \end{bmatrix} \begin{bmatrix} 2.0 \\ 1.0 \\ 4.0 \\ 1.0 \end{bmatrix} = \begin{bmatrix} 1.0 \cdot 2.0 \\ 1.0 \cdot 1.0 \\ \frac{10}{9}(4.0) - \frac{10}{9}(1.0) \\ 1.0 \cdot 4.0 \end{bmatrix} = \begin{bmatrix} 2.0 \\ 1.0 \\ \frac{30}{9} \\ 4.0 \end{bmatrix}
$$

### Passo 3: Divisione prospettica hardware (NDC Space)

La GPU divide per $w_{\text{clip}} = 4.0$:


$$
\mathbf{v}_{\text{NDC}} = \begin{bmatrix} x_{\text{clip}} / w_{\text{clip}} \\ y_{\text{clip}} / w_{\text{clip}} \\ z_{\text{clip}} / w_{\text{clip}} \end{bmatrix} = \begin{bmatrix} 2.0 / 4.0 \\ 1.0 / 4.0 \\ (30/9) / 4.0 \end{bmatrix} = \begin{bmatrix} 0.5 \\ 0.25 \\ \frac{30}{36} \end{bmatrix} = \begin{bmatrix} 0.5 \\ 0.25 \\ 0.833 \end{bmatrix}
$$

### Interpretazione del risultato:

1. Le coordinate $X=0.5$ e $Y=0.25$ cadono all'interno di $[-1, 1]$: il punto è visibile nel quadrante in basso a destra dello schermo.
2. La coordinata orizzontale originale $x=2.0$ è stata scalata a $0.5$ proprio perché divisa per la distanza $z=4.0$.
3. La coordinata di profondità $Z_{\text{NDC}} \approx 0.833$ ricade nell'intervallo $[0, 1]$ di Vulkan. Notare come a $z=4.0$ (appena il $33\%$ della distanza lineare tra $n=1$ e $f=10$), il valore di profondità nel buffer abbia già consumato oltre l'**$83\%$ dell'intero range numerico**, evidenziando concretamente la forte compressione non lineare dei valori lontani.

---

Per visualizzare graficamente la forma del frustum, la deformazione dello spazio e la curva iperbolica del depth buffer: