# Aquarium
Gestion aquarium à base d'Arduino

Premiere version d'un pilotage d'aquarium a base d'arduino.
Archi : 
- Arduino Mega 2560
- Shield Ethernet Arduino
- Sonde pH
- Sonde GH
- 3 sondes de température (One Wire Dallas) (Haut, Bas, interne boitier)
- 4 relais 220v (Eclairage secondaire, bulleur, Doctor Chihiro)
- 1 sonde distance HC-SR04 (Niveau de l'eau)
- 1 IRF520 MOSFET (Dimmer Lampe LED)


Pilotage de l'arduino via GET URL méthode API
 - cmd : commande 
       - debugLevel
       - setRelais
       - setLightStatus
       - getLightStatus
       - setLightTime
       - getLightTime
       - getInfo

- day : selection du jour pour les commandes
       - setLightTime
       - getLightTime

- status : selection du status generalement on(1)/off(0) pour les commandes
       - setRelais
       - setLightStatus
       - setLightTime
       - getLightTime

hour : Heure pour les commande (format String HH:MM:SS)
       - setLightTime

param : parametrage generique
       - debugLevel
       - getInfo (récupération d'information unitaire)
 
relaisPosition : Selection du relais à piloter
       - setRelais

clearLigthTime : suppression de toutes les consignes de la lampe
  

ToDo : sécuriser les appels API en vérifiant la présence de tous les paramètres nécessaire ainsi que leur type pour exécuter les commandes.
