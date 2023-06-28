# Serra Idroponica NFT

Nicola Cremonesi

Realizzazione di una Serra Idroponica di tipo NFT (Nutrient Film Technique). Questa tecnica si basa sul continuo approvvigionamento e riciclo d'acqua alle radici delle piante.
Una pompa pesca l'acqua dal serbatoio e la porta in un tubo dove le piante sono sospese. L'acqua è ossigenata da una pompa ad aria.
Lo stato dell'acqua è monitorato da un sensore per il PH ed uno per l'EC. L'ambiente è controllato da sensori di temperatura e umidità.
Le piante ricevono la luce da un LED a spettro completo. Il flusso dell'acqua è verificato costantemente per evitare, che in caso di guasto le piante secchino.
Per diminuire la temperatura è previsto un sistema di aereazione.
L'utente può decidere i parametri con i quali attivare i diversi sistemi. Per esempio può decidere in base alla tipologia della pianta di accendere e spegnere la luce per determinati periodi in modo da soddisfare le sue esigenze. Oppure impostare dei valori soglia per ricevere una notifica. 
I dati della serra vengono inviati tramite il protocollo MQTT, l'utente può controllare lo stato della serra in ogni momento. 

GNU GENERAL PUBLIC LICENSE version 3 (GPLv3)

# Sensori:
- Temperatura e Umidità  AM2320
- PH
- EC
- Galleggiante (livellostato)


# Attuatori:
- Pompa acqua 
- Pompa aria 
- Ventola
- LED full spectrum
- Qualcosa per scaldare (Futuro)


# Materiali generici:
- Airstone
- Legno
- Tubi PVC
- Serbatoio (scatola)
- Acqua
- Materiale per sotto piantina (clay pebbles, coco coir , rock wool)
- Tubi gomma
- Raccorderia


# Materiali Elettrici
- ESP
- CN1584 (step down regolabile)
- Modulo Relè
- Alimentazione
- Cavi e Cavetti

E' controllabile e impostabile da remoto attraverso messaggi MQTT.
La serra ha una funzione di sincronizzazione con il server MQTT per riprendere gli stati della luce in caso di caduta dell'alimentazione.
Ha un pulsante virtuale di sicurezza per comandare la serra manualmente ed uno fisico per metterla in attesa.


