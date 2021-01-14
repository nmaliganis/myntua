sudo apt get intall git git

Βήματα:
1. Αρχικά θα πρέπει να κατεβάσουμε τα απαραίτητα αρχεία για το κτίσιμο του toolchain από το αντίστοιχο github repository. Εκτελούμε την εντολή
git clone https://github.com/crosstool ng/crosstool ng.git
Η εντολή θα δημιουργήσει στο directory που την εκτελέσαμε έναν φάκελο με όνομα cr osstool
ng.
2. Μπαίνουμε στο φάκελο, και αρχικά εκτελούμε
:~/crosstool ng$ ./boostrap
3. Στη συνέχεια, θα πρέπει να δημιουργήσουμε δύο φακέλους στο HOME directory μας ως εξής:
:~/crosstool ng$ mkdir $HOME/crosstool && mkdir $HOME/src
Στον πρώτο φάκελο θα εγκατασταθεί το πρόγραμμα crosstool-ng ενώ στον δεύτερο, θα αποθηκεύει τα απαραίτητα πακέτα που κατέβαζει για να χτίσει τον cross-compiler.
4. Εκτελούμε την παράκατω εντολή για να κάνουμε configure την εγκατάσταση του crosstool-ng :~/crosstool ng$ ./configure prefix=${HOME}/crosstool
Κατά τη διάρκεια της εκτέλεσης αυτής της εντολής θα εμφανιστούν πολλά πακέτα που λείπουν.