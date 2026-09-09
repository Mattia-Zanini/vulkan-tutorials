### 1. Perché si chiama moveInPlaneXZ se posso muovermi anche su e giù?

Il nome del metodo keyboard_movement_controller.cpp non significa che non puoi muoverti
sull'asse Y (l'altezza), ma indica come viene calcolato il movimento in avanti, indietro,
a destra e a sinistra.

In un gioco FPS (come Minecraft o Call of Duty), se tu guardi dritto in alto verso il
cielo e premi il tasto "Avanti" (W), il tuo personaggio non vola verso il cielo, ma
cammina in avanti sul pavimento.
Questo accade perché i vettori direzionali "Avanti" e "Destra" sono costretti a rimanere
piatti, paralleli al suolo, ovvero vincolati al piano XZ. La loro componente Y viene
forzatamente impostata a 0.

Certo, puoi comunque muoverti in alto o in basso (usando ad esempio i tasti Q ed E), ma
questi movimenti sono calcolati in modo "assoluto" (sempre dritti verso l'alto o verso il
basso), e non dipendono da dove stai guardando. Se invece avessi una telecamera "da
astronave" o da spettatore libero (in cui premendo W voli nella direzione esatta in cui
stai guardando, cielo compreso), non sarebbe più limitata al piano XZ, ma si muoverebbe
nello spazio XYZ.
──────

### 2. Come funzionano i calcoli matematici del movimento?

Tutto si basa sulla Trigonometria applicata alla direzione in cui stai guardando (la
rotazione sull'asse Y, chiamata yaw).

Ecco cosa fa il codice riga per riga per calcolare le direzioni:

#### A. Il vettore forwardDir (Avanti)

    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};

Il valore yaw rappresenta quanto hai ruotato la testa a destra o a sinistra. Immagina un
cerchio disegnato sul pavimento (il piano XZ).

• sin(yaw) ci dà lo spostamento sull'asse X.
• cos(yaw) ci dà lo spostamento sull'asse Z.
• La Y è 0.f. Questo è il motivo principale del nome "PlaneXZ": non importa quanto guardi
in alto (pitch), quando vai "avanti" ti muovi solo in orizzontale.

#### B. Il vettore rightDir (Destra)

    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};

Per spostarti di lato ("strafing"), hai bisogno di un vettore perpendicolare (a 90 gradi)
rispetto a quello "Avanti".
In matematica vettoriale 2D, per ruotare un vettore (x, z) di 90 gradi in senso orario
basta invertire le coordinate e cambiare di segno una delle due, ottenendo (z, -x). La Y
rimane sempre 0.f.

#### C. Il vettore upDir (Alto)

    const glm::vec3 upDir{0.f, -1.f, 0.f};

In Vulkan, convenzionalmente, l'asse Y punta verso il basso (i numeri positivi vanno
verso il pavimento). Quindi, se vogliamo un vettore che punti verso l'alto (verso il
cielo), dobbiamo usare -1.f sull'asse Y, mentre X e Z restano 0.

#### D. Combinazione e Normalizzazione

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
    }

Quando l'utente preme i tasti (es. W e D contemporaneamente), i vettori vengono sommati
dentro la variabile moveDir.

1. Normalizzazione (glm::normalize): Se sommi il movimento "Avanti" (lungo 1) e "Destra"
   (lungo 1), ottieni un vettore diagonale di lunghezza 1.41 (per il teorema di Pitagora).
   Senza la normalizzazione, muoversi in diagonale ti farebbe correre il 41% più veloce!
   normalize riporta la lunghezza complessiva del vettore esattamente a 1.
2. Delta Time (dt): Moltiplicando per dt, ti assicuri che l'oggetto si sposti alla stessa
   velocità reale indipendentemente da quanti FPS fa il tuo computer (un computer a 144Hz
   farà 144 passi piccoli, uno a 60Hz farà 60 passi più grandi, ma la distanza percorsa al
   secondo sarà identica).

Spero che questo chiarisca il mistero dei movimenti e del perché l'autore ha chiamato il
metodo moveInPlaneXZ! Fammi sapere se c'è un passaggio matematico che vuoi esplorare più
a fondo.
