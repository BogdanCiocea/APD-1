Ciocea Bogdan-Andrei 333CA

Tema 1a APD

Pentru implementarea acestei teme am folosit 2 structuri:
-- prima e File care reprezinta un fisier cu id, nume (evident), marimea lui
   exprimata in numar de caractere si cuvintele din acel fisier

-- a doua este acea de Threads, care contine toate campurile posibile
   si imposibile de la mapperi si reduceri in comun (adica mi-a fost prea lene
   sa fac 2 structuri pentru mapperi si reduceri separate)
   Aceasta structura vine la pachet cu fisierele pe care le proceseaza,
   word_map care e practic o lista de rezultate ale thread-urilor de mapperi
   care nu e agregata, ci mai mult e o combinatie de rezultate ale thread-urilor
   de mapperi, bariera, deoarece ca sa fac ca reducerii sa astepte
   mapperii si ca aceste thread-uri sa mearga din acelasi "for", trebuia ca
   mapperii sa se duca la finalul functiei de mapare intr-o bariera, iar reducerii
   trebuia sa se duca la bariera in prima parte a functiei de thread ca sa astepte
   mapperii. De asemenea, am mai adaugat start_letter, end_letter (astea 2 pentru
   reduceri ca sa lucreze fiecare thread de reducer pe un interval fix de litere
   din alfabet), file_indices care ajuta la maparea cuvintelor la fisiere, iar
   folosinta ei a fost ca aveam nevoie sa impart fisierele intre thread-urile
   de mapperi, iar thread-urile foloseau cele mai "grele" fisiere din punctul
   de vedere al marimii si de duceau de la cele mai mari fisiere la cele mai mici
   (din exemplul dat la documentatia/cerinta temei unde era exemplul ala cu 5MB,
   5MB, 1MB, 1MB, 1MB, 1MB). In final, aceasta structura are o variabila care e
   partajata si ea, "current_index", care arata indicele de la fisierele pe care
   vrem ca thread-urile de mapperi sa le ea, acesta incrementandu-se, intre
   mutex-uri, imediat ce un thread de mapper l-a luat.
   

Implementarea temei:


** Main
Hai sa o pornim de la inceput. Suntem in functia main. Acum, noi o sa luam
de la input numarul de mapperi, de reduceri si numele fisierului pe care il
citim initial. Noi pe urma o sa folosim fopen ca sa citim din acest fisier. Luam
numarul de fisiere de unde o sa facem operatiile de mapare si de reducere, iar pe
urma, o sa citim fisierele mai profund si o sa le bagam in vectorul de fisiere.
Prima data o sa ii setam id-ul fisiereului care incepe de la 1 intotdeauna si
nu de la 0. O sa citim fisierul folosind variabila path, intrucat ce scrie in
primul fisier dat de la input sunt caile fisierelor cu tot cu numele lor.
O sa avem o alta variabila "word" care preia cuvintele din fisierul citit si o sa
il adaugam in vectorul de fisiere cu tot cu lungimea lui care se va adauga la
marimea fisierului. Acum avem toate fisierele citite si le putem da mai departe la
thread-uri sa se joace cu ele. Acum, inainte de asta, mai trebuie inca ceva. In
implementarea mea am folosit un vector de indici care pastreaza o ordine a indicilor
de la fisierele din vectorul de fisiere cu marimile cele mai mari la cele mai mici.
Functia "iota" a ajutat sa construiesc acest vector de indici. Si, dupa aia, am
sortat vectorul de indici dupa marimea fisierelor din vectorul de fisiere. Pe urma,
foarte important, am facut un vector de Threads si am format un numar de reduceri.
Eu l-am format asa deoarece ar fi prea greu sa formez in mod egal pentru fisiere
de marimi diferite sa le impart intre thread-urile de reduceri. Asa ca am facut
sa se imparta in mod alfabetic, indiferent de marimea fisierelor, intrucat
operatiile de reduceri se fac pur paralel si nu exista mecanism de asteptare (mutex).
(bine....exista bariera dar aia e asa la inceput....nu se pune >:( ).
Ultima parte din main consta in crearea mapperilor si a reducerilor. Mapperii se
formeaza primii, deci o sa se faca primii, iar reducerii pe urma. Variabila
"reducer_index" e mai mult acolo ca sa impart in mod egal pe alfabet thread-urile
de reduceri. Iar la ultimele linii, fac join la thread-uri si distrug atat mutexul
cat si bariera.

** Mapper
Pentru mapperi e destul de simplu de explicat, o sa vreau sa imi fac un map
local de cuvinte pe care sa le adaug la map-ul "global" de cuvinte. Apoi, cum
a fost descris si in cerinta temei, aceste thread-uri sunt formate dinamic, adica
ele nu stau niciodata pe loc degeaba, ci incearca sa preia cat mai multe din task-uri
pe rand. Aici, am format un while loop ca sa le fac sa nu stea degeaba si sa preia
din task-uri daca ele si-au terminat acel task la care lucrau si sa mai ia din
sarcinile celorlalte thread-uri (cred ca e destul de bine explicat si in cod).
Asa, acum, aici sunt formate operatii critice, adica care nu trebuie sa fie alterate
de alte thread-uri de mapperi sa se faca race condition sau mai stiu eu ce, asa ca
o sa pun operatiile de incrementare a indicelui de la vectorul de indici intre
operatiile de mutex (lock si unlock). Pe urma, odata ce am aflat indicele si
pornesc cu el la citirea fisierului de pe indicele respectiv, preiau fisierul si
adaug la map-ul meu local cuvantul ca si cheie si ca valoare adaug in setul meu
id-ul fisierului respectiv. Am folosit set intrucat el tine valorile unice si
sortate deci e misto. La finalul acestei functii, se adauga pe map-ul global valorile
din map-urile locale ale thread-urilor de mapperi, dar nu se vor duce agregate,
ci fiecare mapper o sa aiba locul lui in vectorul de word_map (tot intre
operatiile de mutex). Thread-urile de mapperi se duc la bariera si asteapta ca
toate thread-urile de mapperi (si de reduceri) sa ajunga la bariera.

** Reducer
La aceasta parte, a trebuit sa lucrez sa ajung sa nu am mutex-uri ca sa
se poata scala cum trebuie programul (adica mici dureri de cap). Buuuun, ce am facut
aici...Aici stim asa, ca avem o lista deja facuta, dar noi vrem sa o
impartim in mai multe thread-uri de reduceri. Asa ca solutia e sa le adaugam
intr-un vector de perechi de cuvinte - vector de file id-uri si sa le adaugam destept
(facute intr-un interval de la o litera din alfabet de start pana la una de sfarsit,
intrucat noi vrem sa paralelizam acest proces). Am preluat aceste date in
vectorul "local_word_map" si apoi o sa il sortam mai intai dupa numarul de file
id-uri care se regasesc in acel set si pe urma alfabetic. Pe urma e ceva banal, parcurgem
acele litere din alfabet si cream fisierele de final in care bagam aceste
perechi. Si pe urma se iese din functia de reduce....si cam asta e pe scurt.

Note, sanse...etc

Tema a fost una draguta, dar e pacat ca a fost intr-o perioada incarcata.