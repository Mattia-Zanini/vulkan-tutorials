#include <SDL3/SDL_main.h>
#include "application.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);

    // L'incapsulamento dell'intero codice all'interno di una singola classe Application non è una 
    // prescrizione rigida sull'architettura, ma un approccio semplice per suddividere le fasi 
    // di inizializzazione ed evitare di riempire lo scope con una miriade di variabili e funzioni globali.
    Application app;
    app.run();

    return 0;
}