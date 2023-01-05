# Raduta Lavinia-Maria 333CA
# Algoritmi Paraleli si Distribuiti - Tema 3
# Calcule Colaborative in Sisteme Distribuite

1. Flow-ul general al programului
- indiferent de erorile care pot sa apara in topologie, programul urmareste
acelasi flow:
    - **spread_N**
        - coordonatorul 0 este singurul care stie initial valoarea N, pe care o
        afla din argumentele din linia de comanda
        - trimite celorlalti coordonatori valoarea N respectand topologia 
        inelului
    - **spread_coordinators**
        - fiecare coordonator citeste propriul fisier de intrare si completeza 
        vectorul de coordonatori cu informatia proprie
        - fiecare coordonator de cluster anunta proprii workeri ca el este
        coordonatorul lor
    - **complete_topology**
        - se completeaza intreaga topologie
        - din aproape in aproape fiecare coordonator completeaza prpriile 
        informatii despre topologie (care sunt workerii lui) si trimite mai 
        departe informatia
        - la procesul 0 o sa ajunga pentru prima oara topologia completa care 
        poate fi afisata
    - **spread_topology**
        - procesul 0 trimite topologia catre workerii lui
        - procesul 0 incepe sa trimita in inel topologia catre celelalte 
        clustere, iar coordonatorii trimit fiecare catre workerii lor
        - cand un proces primeste topologia, o afiseaza
    - **count_workers**
        - stiind topologia completa, putem afla care este numarul de workeri 
        disponibili pentru realizarea calculelor
        - astfel se poate calcula si numarul de calcule pe care trebuie sa le 
        faca fiecare worker
    - **spread_work**
        - procesul 0 genereaza vectorul pe care trebuie aplicate calculele
        - procesul 0 incepe sa trimita in inel vectorul catre celelalte 
        clustere, iar coordonatorii trimit fiecare catre workerii lor doar 
        bucata de care acestia trebuie sa se ocupe
        - coordonatorul afla o data cu primirea vectorului si indexul global al 
        primului sau worker (deci indecsii tururor workerilor). asta este 
        important pentru impartirea vectorului pe care se aplica calculele. 
        fiecare worker avand un index, poate afla indexul de start si de end al 
        bucatii de care se ocupa
        - deoarece toti coordonatorii de cluster cunosc cate calcule vor face 
        fiecare dintre workeri, calculele vor fi impartite echilibrat intre 
        workeri
        - coordonatorul trimite workerilor doar dimensiunea bucatii de care se 
        vor ocupa si doar bucata respectiva
        - dupa ce primesc bucsta asignata, workerii realizeaza calculele si 
        trimit inapoi coordonatorului rezultatul
    - **get_partial_array**
        - coordonatorul primeste rezultatul de la workeri (doar bucata de care 
        acestia s-au ocupat) si completeaza in vectorul initial primit de la 
        procesul 0
        - in acestmoment fiecare coordonator are o solutie partiala
    - **combine_results**
        - coordonatorul 0 trebuie sa primeasca rezultatele partiale si sa le 
        combine obtinand vectorul final
        - acesta va primi rezulatatele celorlalti coordonatori si va le va 
        combina (in vectorul sau rezultat completeaza valorile partiale din 
        vectorul primit)
        - fiecare coordonator intermediar va primi rezultate de la coordonatorul 
        anterior, le trimite catre urmatorul si apoi trimite si el mai departe 
        valorile lui, astfel la fiecare pas un coordonator va trimite cu 1 
        vector mai mult in continuare

2. Eroare de conexiune intre 0 si 1
- daca legatura dintre 0 si 1 este indisponibila vor aparea cateva schimbari in 
algoritmii folositi:
    - informatiile care pornesc de la procesul 0 nu vor mai urmari inelul 
    0-1-2-3-0, ci vor urma calea 0-3-2-1 
    - procesul 1 este cel porneste transmiterea topologiei partiale (in locul 
    lui 0), cat si a vectorilor rezultat partiali
    
3. Partitionare - Izolarea clusterului 1
- daca clusterul 1 este izolat, acesta nu va mai trimite topologia sa partiala, 
ceea ce va duce ca restul grupului de clustere sa aiba o topologie diferita
- cu ajutorul acestei diferente se poate calcula noul numar de workeri 
disponibili pentru realizarea de calcule si numarul de calcule pe care trebuie 
sa le faca fiecare worker
- fiindca este izolat, clusterul 1 nu primeste valoarea lui N
- pentru restul de clustere grupate, comunicarea se face asemanator cu cazul 
precedent, pe directia 0-3-2, 2 devenind sursa pentru topologia partiala si 
pentru rezultatele partiale