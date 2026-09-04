# Guida Completa: Il Significato e l'Uso delle `location` negli Shader (GLSL & Vulkan)

In GLSL e Vulkan, la sintassi `layout(location = X)` è fondamentale per definire **l'interfaccia di comunicazione** tra:
1. **La CPU e la GPU** (tramite la pipeline e i Vertex Buffer).
2. **I diversi stadi della pipeline** (ad esempio dal Vertex Shader al Fragment Shader).
3. **Lo shader e il Framebuffer** (render target di output).

---

## 1. A cosa serve il valore di `location`?

Negli shader compilati (in formato bytecode **SPIR-V** per Vulkan), i **nomi** delle variabili (come `position`, `color`, `fragColor`) vengono rimossi o ignorati durante l'esecuzione sulla GPU. 

La GPU e il driver non associano i dati basandosi sul nome delle variabili nel codice sorgente. Serve quindi un indice numerico univoco ed esplicito per ogni "canale" o slot di dati: questo è la **`location`**.

Possiamo pensare a una `location` come al **numero di una porta o di una casella postale**:
- Il mittente (CPU o stadio precedente) invia dati alla casella `0`.
- Il destinatario (shader o stadio successivo) riceve i dati leggendo dalla casella `0`.
- Il nome effettivo della variabile serve unicamente al programmatore per scrivere il corpo della funzione.

---

## 2. Perché e quando il valore di `location` deve coincidere?

### A. Tra C++ (Pipeline/Model) e Vertex Shader
Nel nostro codice C++, quando configuriamo le descrizioni degli attributi dei vertici:

```cpp
// In lve_model.cpp
attributeDescriptions[0].location = 0; // <--- Location 0
attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;

attributeDescriptions[1].location = 1; // <--- Location 1
attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
```

E nel Vertex Shader:
```glsl
// In simple_shader.vert
layout(location = 0) in vec2 position; // Collegato a attributeDescriptions[0]
layout(location = 1) in vec3 color;    // Collegato a attributeDescriptions[1]
```

Se invertissimo le location nello shader o nel C++, la GPU leggerebbe i byte della posizione considerandoli colori e viceversa, causando rendering errato o crash.

---

### B. Tra Vertex Shader e Fragment Shader
I dati calcolati nel Vertex Shader vengono passati al Fragment Shader attraverso lo stadio di rasterizzazione (che calcola le coordinate baricentriche ed esegue l'interpolazione lineare).

Nel Vertex Shader:
```glsl
// simple_shader.vert
layout(location = 0) out vec3 vColor; // Nome: vColor, Location: 0

void main() {
    vColor = color;
}
```

Nel Fragment Shader:
```glsl
// simple_shader.frag
layout(location = 0) in vec3 inColor; // Nome: inColor (diverso!), Location: 0

void main() {
    outColor = vec4(inColor, 1.0);
}
```

> **Regola fondamentale:**
> **I nomi possono differire, ma la `location` e il tipo di dato DEVONO coincidere.**
> Il linker grafico associa l'output del Vertex Shader all'input del Fragment Shader unicamente tramite la corrispondenza `location = 0` e tipo `vec3`.

---

## 3. Quante `location` occupa un tipo di dato? (Slot a 16 byte e tipi a 64 bit / Matrici)

Una singola `location` in Vulkan/GLSL fornisce uno slot di **16 byte (ovvero un vettore da 4 componenti a 32 bit, come un `vec4`)**:

| Tipo di Dato GLSL | Dimensione in byte | Slot `location` occupati | Note |
| :--- | :--- | :--- | :--- |
| `float`, `int`, `uint` | 4 byte (1 componente) | **1** | Rimangono componenti inutilizzate nello slot |
| `vec2`, `ivec2` | 8 byte (2 componenti) | **1** | |
| `vec3`, `vec4` | 12 / 16 byte (3/4 componenti) | **1** | Riempie uno slot intero |
| `double`, `dvec2` | 8 / 16 byte (a 64-bit) | **1** | Componenti a precisione doppia |
| `dvec3`, `dvec4` | 24 / 32 byte | **2** | **Supera i 16 byte, consuma 2 location consecutive!** |
| `mat4` | 64 byte (4 x vec4) | **4** | 4 colonne da 16 byte ciascuna $\rightarrow$ 4 location |

### Esempio: Matrici e tipi a 64 bit

Se passi una matrice $4 \times 4$ come attributo di vertice:
```glsl
layout(location = 0) in mat4 modelMatrix; // Occupa le location 0, 1, 2, 3!
layout(location = 4) in vec3 normal;      // La successiva DEVE essere 4, non 1!
```

Se passi un `dvec3` (3 double da 64 bit = 24 byte):
```glsl
layout(location = 0) in dvec3 preciseCoord; // Occupa le location 0 e 1!
layout(location = 2) in vec2 texCoord;      // La successiva deve essere 2!
```

---

## 4. Perché `in` e `out` possono usare gli STESSI valori di `location`?

Nello stesso vertex shader vediamo:
```glsl
layout(location = 0) in vec2 position;   // Input alla location 0
layout(location = 0) out vec3 fragColor; // Output ALLA STESSA location 0!
```

### Namespace e Registri Hardware Separati

In Vulkan e nella pipeline grafica della GPU, gli input e gli output di uno shader appartengono a **spazi dei nomi e registri hardware completamente separati e indipendenti**:

```
[ CPU / Vertex Buffer ]
         │
         ▼ (Interfaccia di INPUT del Vertex Shader)
   location = 0  ──►  in vec2 position
   location = 1  ──►  in vec3 color

----------------------------------------------------
             [ VERTEX SHADER MAIN ]
----------------------------------------------------

   out vec3 fragColor  ──►  location = 0
         ▲
         │ (Interfaccia di OUTPUT del Vertex Shader)
         ▼
[ Rasterizer (Interpolazione Baricentrica) ]
         ▼
         │ (Interfaccia di INPUT del Fragment Shader)
   location = 0  ──►  in vec3 fragColor

----------------------------------------------------
            [ FRAGMENT SHADER MAIN ]
----------------------------------------------------

   out vec4 outColor   ──►  location = 0
         ▲
         │ (Interfaccia di OUTPUT del Fragment Shader)
         ▼
[ Color Attachment 0 del Framebuffer ]
```

1. **`in` nel Vertex Shader**: rappresenta i registri di lettura dai buffer di vertici inviati dalla CPU.
2. **`out` nel Vertex Shader**: rappresenta i registri di scrittura verso il rasterizzatore hardware.
3. **`in` nel Fragment Shader**: rappresenta i registri di lettura dei valori interpolati dal rasterizzatore.
4. **`out` nel Fragment Shader**: rappresenta a quale **color attachment del Framebuffer** scrivere il pixel finale (es. l'attachment 0 descritto nel Render Pass).

Poiché le porte fisiche di lettura e di scrittura dello shader sono canali distinti, **non c'è alcuna collisione**: `in` ha il suo elenco di location che parte da 0, e `out` ha il suo elenco di location che parte anch'esso da 0.

---

## Sintesi

- **A cosa serve `location`:** È l'ID numerico del canale di trasmissione del dato che sostituisce i nomi delle variabili dopo la compilazione in SPIR-V.
- **Perché deve coincidere:** Perché il linker della pipeline e il driver grafico uniscono gli stadi cercando lo stesso ID numerico e un tipo compatibile.
- **Consumo di slot:** Ogni location ospita fino a 16 byte (vettori a 32 bit a 4 elementi). Dati più grandi (es. `mat4` o `dvec3`) occupano più location consecutive.
- **Uguaglianza tra `in` e `out`:** L'interfaccia di input e quella di output sono separate; per questo motivo entrambe possono numerare i propri slot partendo da 0 senza andare in conflitto.
