# Trasformazioni della Camera e Matrici di Proiezione: Da Spazio Oggetto a Pixel

Questo documento spiega la catena di azioni matematiche necessarie per prendere un oggetto 3D (o 2D) con le sue coordinate originali e disegnarlo correttamente sui pixel del nostro schermo. Come spiegato nel **Tutorial 14 (Camera Transform)**, queste operazioni avvengono attraverso una sequenza ben precisa di trasformazioni matematiche attraverso le matrici.

---

## L'Illusione della Fotocamera (The View Transform)

La trasformazione della telecamera (nota anche come **View Transform** o **Eye Transform**) è ciò che ci permette di osservare il mondo di gioco da qualsiasi punto di vista e angolazione. 

Come spiegato dal ragazzo del tutorial: *"È come se potessimo posizionare una telecamera immaginaria all'interno del mondo, muoverla, catturare ciò che vede e mostrarlo sullo schermo."*

In realtà, in grafica computerizzata **la telecamera fisicamente non si muove mai**. Essa rimane sempre fissa al centro dell'universo (nell'origine `(0,0,0)`) e guarda verso una direzione fissa. Per simulare il movimento della telecamera in avanti, **spostiamo l'intero mondo all'indietro**. Se ruotiamo la telecamera a destra, **ruotiamo tutto il mondo a sinistra**. La *View Matrix* calcola proprio questa complessa "trasformazione inversa" del mondo.

---

## La Catena delle Trasformazioni (The Transformation Pipeline)

Per portare un vertice dalla sua posizione originale fino al tuo schermo, applichiamo in sequenza delle moltiplicazioni tra matrici. Ecco le fasi del viaggio di un vertice:

### 1. Object Space (o Local Space)
Le coordinate originali in cui l'oggetto è stato disegnato o modellato (ad esempio le coordinate del tuo Sierpinski triangle). L'origine `(0,0,0)` è solitamente il centro esatto dell'oggetto stesso.

⬇️ *Applicazione della **Model Matrix***

### 2. World Space
La **Model Matrix** trasla (sposta), ruota e scala l'oggetto, posizionandolo nel mondo di gioco. Ora i vertici non sono più relativi al centro dell'oggetto, ma all'origine globale (il centro dell'intero mondo di gioco).

⬇️ *Applicazione della **View Matrix** (Camera Transform)*

### 3. Camera Space (o Eye Space / View Space)
Qui entra in gioco il Tutorial 14. La **View Matrix** prende tutto ciò che si trova nel mondo e lo sposta affinché le sue coordinate diventino relative alla posizione della nostra telecamera immaginaria. Ora l'origine `(0,0,0)` non è più il centro del mondo, ma coincide col sensore o l'obiettivo della nostra telecamera.

⬇️ *Applicazione della **Projection Matrix***

### 4. Clip Space (Canonical View Volume)
Successivamente entra in gioco la **Projection Matrix** (che può essere di tipo Ortografico o Prospettico).
Questa matrice compie due azioni principali:
- **Prospettiva:** Distorce lo spazio in modo che gli oggetti più lontani appaiano più piccoli, creando la sensazione di profondità (se si usa una proiezione prospettica).
- **Normalizzazione (Clip):** Comprime tutto lo spazio visivo (il *Frustum*) all'interno di un cubo di dimensioni standardizzate. In Vulkan, lo schermo va da `-1.0` a `1.0` per X e Y, e da `0.0` a `1.0` per la profondità Z. Tutti i vertici che finiscono fuori da queste coordinate vengono eliminati o "tagliati via" (*Clipped*), poiché significa che sono dietro di noi o oltre i bordi dello schermo.

⬇️ *Applicazione della **Viewport Transform***

### 5. Screen Space
Infine, le coordinate "standardizzate" o normalizzate vengono mappate (dietro le quinte dalla pipeline grafica di Vulkan) direttamente sui pixel fisici della tua finestra o del tuo schermo (es. da `0` a `1920` per la larghezza in pixel, ecc.).

---

## Riepilogo Matematico nel Codice

Se andiamo a guardare come tutte queste fasi si traducono nel codice del nostro **Vertex Shader** (`.vert`), l'intero viaggio si riassume in una singola istruzione composta da una catena di moltiplicazioni:

```glsl
// La posizione finale del vertice viene calcolata applicando la catena di matrici
// L'ordine di moltiplicazione si legge da DESTRA verso SINISTRA
gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
```

Spiegazione dell'ordine (da destra a sinistra):
1. Si prende la posizione base del vertice (`inPosition`).
2. Lo si moltiplica per la `modelMatrix` (Object Space ➔ World Space).
3. Il risultato viene moltiplicato per la `viewMatrix` (World Space ➔ Camera Space).
4. Infine, viene moltiplicato per la `projectionMatrix` (Camera Space ➔ Clip Space).
