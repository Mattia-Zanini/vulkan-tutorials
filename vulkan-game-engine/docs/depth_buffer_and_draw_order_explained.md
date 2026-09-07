# Approfondimento: Depth Buffer, Ordine di Disegno e Sovrapposizione 2D in Vulkan

Quando si renderizzano più oggetti 2D orientati e posizionati sullo stesso piano (con coordinata $Z = 0$), ci si aspetterebbe intuitivamente che l'ultimo oggetto disegnato copra quelli precedenti, oppure che oggetti più grandi nascondano completamente quelli più piccoli.

Nel nostro motore grafico, tuttavia, osserviamo una **pila di triangoli rotanti** dove il triangolo **più piccolo si trova in cima a tutti** e non viene mai coperto dai triangoli più grandi successivi.

Questo documento spiega nel dettaglio l'interazione tra:
1. **L'ordine di creazione e disegno degli oggetti (Painter's Algorithm)**
2. **Lo stato del Depth Buffer e il clear iniziale**
3. **L'operatore di confronto `VK_COMPARE_OP_LESS`**

---

## 1. Come vengono generati e disegnati i Triangoli

Nel nostro file `first_app.cpp`, la funzione `loadGameObjects()` istanzia 40 entità assegnando a ciascuna una scala progressivamente crescente:

```cpp
// In first_app.cpp -> loadGameObjects()
for (int i = 0; i < 40; i++) {
    auto triangle = LveGameObject::createGameObject();
    triangle.model = lveModel;
    triangle.color = colors[i % colors.size()];
    
    // Il triangolo i=0 ha scala 0.500 (il più piccolo)
    // Il triangolo i=39 ha scala 1.475 (il più grande)
    triangle.transform2d.scale = glm::vec2(0.5f) + i * 0.025f;
    triangle.transform2d.rotation = i * glm::pi<float>() * 0.025f;

    gameObjects.push_back(std::move(triangle));
}
```

Successivamente, ad ogni frame, la funzione `renderGameObjects()` invia i comandi di disegno alla GPU iterando lungo il vettore `gameObjects`:

```cpp
// In first_app.cpp -> renderGameObjects()
for (auto& obj : gameObjects) {
    // ... invio push constants ...
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
}
```

Poiché il ciclo scorre gli elementi in ordine crescente dall'indice `0` a `39`, **il triangolo più piccolo (`gameObjects[0]`) viene sempre inviato alla GPU e rasterizzato per PRIMO**.

---

## 2. La coordinata di Profondità nello Shader

Nel nostro vertex shader `simple_shader.vert`, la posizione finale del vertice viene calcolata così:

```glsl
#version 450

layout(location = 0) in vec2 position;
// ...
layout(push_constant) uniform Push {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() {
    // Nota la coordinata Z fissata a 0.0 per tutti i vertici:
    gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
}
```

Tutti i vertici di tutti i 40 triangoli producono frammenti con la **stessa identica coordinata di profondità ($Z = 0.0$)**.

---

## 3. La Configurazione del Depth Test nella Pipeline

Nella configurazione predefinita della pipeline grafica (`LvePipeline::defaultPipelineConfigInfo`), abbiamo impostato:

```cpp
// In lve_pipeline.cpp
configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // <--- FONDAMENTALE!
```

- **`depthTestEnable = VK_TRUE`**: Per ogni frammento generato dal rasterizer, la GPU controlla il valore presente nel Depth Buffer alla coordinata $(x, y)$ del pixel prima di colorarlo.
- **`depthWriteEnable = VK_TRUE`**: Se il test ha successo, la nuova profondità viene scritta nel Depth Buffer sovrascrivendo quella vecchia.
- **`depthCompareOp = VK_COMPARE_OP_LESS`**: Il frammento viene disegnato **SOLO SE** la sua profondità è **STRETTAMENTE MINORE (`<`)** rispetto a quella già presente nel buffer. Se è uguale o maggiore, **il frammento viene scartato**.

---

## 4. Passo dopo Passo: Cosa accade sulla GPU

| Fase | Operazione | Valore Depth Buffer nell'area del triangolo piccolo | Risultato sul Framebuffer |
| :--- | :--- | :--- | :--- |
| **0. Clear iniziale** | `clearValues[1].depthStencil = { 1.0f, 0 };` | `1.0` (il punto più lontano) | Framebuffer ripulito con il colore di sfondo scuro |
| **1. Triangolo 0 (piccolo)** | Viene rasterizzato per primo con profondità $Z = 0.0$. Test: $0.0 < 1.0$ (TRUE). | Scrive `0.0` su tutti i pixel coperti dal triangolo. | I pixel del triangolo piccolo vengono disegnati. |
| **2. Triangolo 1 (medio)** | Viene rasterizzato subito dopo con profondità $Z = 0.0$. | | |
| ↳ *Pixel centrali (sovrapposti al triangolo 0)* | Test: $0.0 < 0.0$ ➔ **FALSO** | Rimane `0.0` | **Scartati!** Il triangolo piccolo non viene toccato. |
| ↳ *Pixel esterni (non ancora coperti)* | Test: $0.0 < 1.0$ ➔ **TRUE** | Scrive `0.0` | **Disegnati!** Compare la porzione esterna del triangolo 1. |
| **3. Triangoli successivi fino a 39** | Stesso identico comportamento: i pixel già colorati hanno profondità `0.0` e non possono essere sovrascritti. | Rimane `0.0` ovunque sia già passato un triangolo. | Solo le parti più esterne dei triangoli più grandi vengono disegnate. |

---

## 5. Il Confronto: `VK_COMPARE_OP_LESS` vs `VK_COMPARE_OP_LESS_OR_EQUAL`

Questo comportamento è determinato specificamente dalla scelta di `VK_COMPARE_OP_LESS`:

1. Con **`VK_COMPARE_OP_LESS`** (il nostro caso):
   - $0.0 < 0.0$ è **FALSO**.
   - Vince **chi arriva per primo**. Il primo oggetto che scrive `0.0` "prenota" i suoi pixel e blocca tutti quelli che provano a ridisegnare alla stessa quota $Z = 0.0$.
   - Risultato visivo: il triangolo più piccolo (disegnato per primo) appare davanti a tutti.

2. Se usassimo **`VK_COMPARE_OP_LESS_OR_EQUAL`**:
   - $0.0 \le 0.0$ è **VERO**.
   - Vince **l'ultimo arrivato** (*Painter's Algorithm standard*).
   - Ogni nuovo triangolo sovrascriverebbe i pixel di quelli precedenti.
   - Risultato visivo: il triangolo più grande (disegnato per ultimo) coprirebbe interamente tutti i triangoli più piccoli sotto di esso!

3. Se **disabilitassimo il Depth Test** (`depthTestEnable = VK_FALSE`):
   - La GPU non userebbe affatto il buffer di profondità.
   - I pixel verrebbero sempre sovrascritti in ordine di disegno, nascondendo i triangoli più piccoli.
