# Modern Vulkan Tutorial (macOS ARM64 Sequoia Port)

Questo progetto è basato sul tutorial "Modern Vulkan" dello youtuber **Nick Enchev**.

- **Video del tutorial:** [Guarda su YouTube](https://youtu.be/DC9FBRQKNck?si=qZPFR5nEznLoMHUW)
- **Repository originale:** [GitHub - modern-vulkan (lesson branch)](https://github.com/nickenchev/modern-vulkan/tree/lesson)

## Descrizione

L'obiettivo di questo progetto è seguire e riprodurre l'apprendimento delle basi e delle tecniche moderne di Vulkan tramite il tutorial sopra citato. 

**Nota importante:** Il tutorial originale è stato concepito e mostrato per un ambiente **Windows**. Tuttavia, questo progetto è stato adattato e viene sviluppato su **macOS ARM64 (Apple Silicon) con macOS 15 Sequoia**. Ci sono quindi alcune differenze di configurazione rispetto all'originale, in particolare per garantire la compatibilità con macOS (che utilizza MoltenVK come livello di traduzione Vulkan-su-Metal).

Il codice è scritto in C++26.

## Dipendenze

Il progetto utilizza `vcpkg` (in modalità manifest tramite `vcpkg.json`) e CMake. Le librerie principali utilizzate sono:
- **Vulkan** (incluso MoltenVK su macOS)
- **SDL3** (per la gestione della finestra e degli input, con supporto Vulkan)
- **GLM** (libreria matematica)
- **Volk** (Vulkan meta-loader)
- **VulkanMemoryAllocator (VMA)** (per l'allocazione della memoria della GPU)
- **Shaderc** (per la compilazione degli shader a runtime/build time)
- **Spdlog & Fmt** (per un logging efficiente e formattato)

## Come buildare il progetto

Essendo un progetto basato su CMake e `vcpkg`, la configurazione è abbastanza standard. Assicurati di avere installati nel tuo sistema:

- [CMake](https://cmake.org/) (versione 3.24 o superiore)
- Un compilatore compatibile con **C++26** (es. Apple Clang aggiornato tramite Xcode, o LLVM/GCC via Homebrew)
- [Vulkan SDK per macOS](https://vulkan.lunarg.com/sdk/home) (fondamentale per MoltenVK e gli strumenti di sviluppo)
- [vcpkg](https://github.com/microsoft/vcpkg) per risolvere automaticamente le dipendenze

### Passaggi per la build da terminale:

1. **Apri il terminale** nella cartella principale del progetto.
2. **Configura il progetto con CMake**:
   Devi indicare a CMake di utilizzare il toolchain di `vcpkg` in modo che possa leggere il file `vcpkg.json` e scaricare le librerie necessarie.
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/percorso/assoluto/del/tuo/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```
   *(Ricorda di sostituire `/percorso/assoluto/del/tuo/vcpkg` con il percorso reale in cui hai installato vcpkg)*

3. **Compila l'eseguibile**:
   ```bash
   cmake --build build
   ```

4. **Esecuzione**:
   Il CMakeLists del progetto è configurato per copiare automaticamente la cartella `shaders` nella directory di build post-compilazione. Puoi avviare l'app con:
   ```bash
   ./build/vulkanapp
   ```

### Sviluppare con VS Code

Se usi Visual Studio Code, ti basterà:
1. Installare le estensioni **CMake Tools** e **C/C++**.
2. Nelle impostazioni di CMake Tools (`cmake.configureArgs` o `cmake.cmakePath`), assicurarti che il toolchain di `vcpkg` sia specificato.
3. Premere **Build** (o ⌘+Shift+B) ed eseguire l'applicazione dal pannello di CMake in basso.
