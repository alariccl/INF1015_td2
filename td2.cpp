﻿////////////////////////////////////////////////////////////////////////////////
/// \file   td3.cpp
/// \author Alaric Chan Lock et Bryan Sanchez
///
/// Les modifications apportés au TD2 en utilisant la matière des chapitres 6 à 10
////////////////////////////////////////////////////////////////////////////////

#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <sstream>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).
using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{
template <typename T>
T lireType(istream& fichier)
{
	T valeur{};
	fichier.read(reinterpret_cast<char*>(&valeur), sizeof(valeur));
	return valeur;
}
#define erreurFataleAssert(message) assert(false&&(message)),terminate()
static const uint8_t enteteTailleVariableDeBase = 0xA0;
size_t lireUintTailleVariable(istream& fichier)
{
	uint8_t entete = lireType<uint8_t>(fichier);
	switch (entete) {
	case enteteTailleVariableDeBase + 0: return lireType<uint8_t>(fichier);
	case enteteTailleVariableDeBase + 1: return lireType<uint16_t>(fichier);
	case enteteTailleVariableDeBase + 2: return lireType<uint32_t>(fichier);
	default:
		erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");  //NOTE: Il n'est pas possible de faire des tests pour couvrir cette ligne en plus du reste du programme en une seule exécution, car cette ligne termine abruptement l'exécution du programme.  C'est possible de la couvrir en exécutant une seconde fois le programme avec un fichier films.bin qui contient par exemple une lettre au début.
	}
}

string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUintTailleVariable(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

// Fonctions pour ajouter un Film à une ListeFilms.
//[
void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film * [nouvelleCapacite];

	if (elements != nullptr) {  // Noter que ce test n'est pas nécessaire puique nElements sera zéro si elements est nul, donc la boucle ne tentera pas de faire de copie, et on a le droit de faire delete sur un pointeur nul (ça ne fait rien).
		nElements = min(nouvelleCapacite, nElements);
		for (int i : range(nElements))
			nouvelleListe[i] = elements[i];
		delete[] elements;
	}

	elements = nouvelleListe;
	capacite = nouvelleCapacite;
}

void ListeFilms::ajouterFilm(Film* film)
{
	if (nElements == capacite)
		changeDimension(max(1, capacite * 2));
	elements[nElements++] = film;
}

//]

// Fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
//[
// On a juste fait une version const qui retourne un span non const.  C'est valide puisque c'est la struct qui est const et non ce qu'elle pointe.  Ça ne va peut-être pas bien dans l'idée qu'on ne devrait pas pouvoir modifier une liste const, mais il y aurais alors plusieurs fonctions à écrire en version const et non-const pour que ça fonctionne bien, et ce n'est pas le but du TD (il n'a pas encore vraiment de manière propre en C++ de définir les deux d'un coup).
span<Film*> ListeFilms::enSpan() const { return span(elements, nElements); }

void ListeFilms::enleverFilm(const Film* film)
{
	for (Film*& element : enSpan()) {  // Doit être une référence au pointeur pour pouvoir le modifier.
		if (element == film) {
			if (nElements > 1)
				element = elements[nElements - 1];
			nElements--;
			return;
		}
	}
}
//]

// Fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
//[

//NOTE: Doit retourner un Acteur modifiable, sinon on ne peut pas l'utiliser pour modifier l'acteur tel que demandé dans le main, et on ne veut pas faire écrire deux versions.
shared_ptr<Acteur> ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (const shared_ptr<Acteur>& acteur : film->acteurs.enSpan()) {
			if (acteur->nom == nomActeur)
				return acteur;
		}
	}
	return nullptr;
}
//]

// Les fonctions pour lire le fichier et créer/allouer une ListeFilms.

shared_ptr<Acteur> lireActeur(istream& fichier, const ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom = lireString(fichier);
	acteur.anneeNaissance = int(lireUintTailleVariable(fichier));
	acteur.sexe = char(lireUintTailleVariable(fichier));

	shared_ptr<Acteur> acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr)
		return acteurExistant;
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		return make_shared<Acteur>(move(acteur));  // Le move n'est pas nécessaire mais permet de transférer le texte du nom sans le copier.
	}
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film* film = new Film;
	film->titre = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->annee = int(lireUintTailleVariable(fichier));
	film->recette = int(lireUintTailleVariable(fichier));
	auto nActeurs = int(lireUintTailleVariable(fichier));
	film->acteurs = ListeActeurs(nActeurs);  // On n'a pas fait de méthode pour changer la taille d'allocation, seulement un constructeur qui prend la capacité.  Pour que cette affectation fonctionne, il faut s'assurer qu'on a un operator= de move pour ListeActeurs.
	cout << "Création Film " << film->titre << endl;

	for ([[maybe_unused]] auto i : range(nActeurs)) {  // On peut aussi mettre nElements avant et faire un span, comme on le faisait au TD précédent.
		film->acteurs.ajouter(lireActeur(fichier, listeFilms));
	}

	return film;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = int(lireUintTailleVariable(fichier));

	ListeFilms listeFilms;
	for ([[maybe_unused]] int i : range(nElements)) { //NOTE: On ne peut pas faire un span simple avec ListeFilms::enSpan car la liste est vide et on ajoute des éléments à mesure.
		listeFilms.ajouterFilm(lireFilm(fichier, listeFilms));
	}

	return listeFilms;
}

// Fonction pour détruire une ListeFilms et tous les films qu'elle contient.
//[
//NOTE: La bonne manière serait que la liste sache si elle possède, plutôt qu'on le dise au moment de la destruction, et que ceci soit le destructeur.  Mais ça aurait complexifié le TD2 de demander une solution de ce genre, d'où le fait qu'on a dit de le mettre dans une méthode.
void ListeFilms::detruire(bool possedeLesFilms)
{
	if (possedeLesFilms)
		for (Film* film : enSpan())
			delete film;
	delete[] elements;
}

void transfererFilms(vector<Item>& bibliotheque, const ListeFilms& listeFilms)
{
	for (Film* film : listeFilms.enSpan()) 
	{
		bibliotheque.push_back(move(*film)); // arranger ici
	}
}

void ajouterLivres(vector<Item>& bibliotheque, string nomFichier)
{
	ifstream fichier(nomFichier);

	if (!fichier.is_open()) 
	{
		cout << "Error opening file." << std::endl;
		return;
	};
	
	Livre* livre = new Livre;
	while (fichier >> quoted(livre->titre) >> livre->annee >> quoted(livre->auteur) >> livre->millionsDeCopiesVendus >> livre->nombresDePages) 
	{
		bibliotheque.push_back(move(*livre));
	}

	delete livre;

	fichier.close();
}

// Pour que l'affichage de Film fonctionne avec <<, il faut aussi modifier l'affichage de l'acteur pour avoir un ostream; l'énoncé ne demande pas que ce soit un opérateur, mais tant qu'à y être...
ostream& operator<< (ostream& os, const Acteur& acteur)
{
	return os << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

// Fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
//[
//ostream& operator<< (ostream& os, const Film& film)
//{
//	os << "Titre: " << film.titre << endl;
//	os << "  Réalisateur: " << film.realisateur << "  Année :" << film.annee << endl;
//	os << "  Recette: " << film.recette << "M$" << endl;
//
//	os << "Acteurs:" << endl;
//	for (const shared_ptr<Acteur>& acteur : film.acteurs.enSpan())
//		os << *acteur;
//	return os;
//}

//ostream& operator<< (ostream& os, const	vector<Item>& biblio)
//{
//	static const string ligneDeSeparation = //[
//		"\033[32m────────────────────────────────────────\033[0m\n";
//	os << ligneDeSeparation;
//	for (int i : range(biblio.size()))
//	{
//		os << biblio[i] << ligneDeSeparation;
//	}
//	return os;
//}

ostream& Item::output(ostream& os) const
{
	os	<< "Titre: " << titre << endl 
		<< "Année : " << annee << endl;
	return os;
}

ostream& operator<< (ostream& os, const Item& item)
{
	return item.output(os);
}

ostream& Film::output(ostream& os) const
{
	os << "Titre: " << titre << endl;
	os << "  Réalisateur: " << realisateur << "  Année : " << annee << endl;
	os << "  Recette: " << recette << " M$" << endl;
	os << "Acteurs:" << endl;
	for (const shared_ptr<Acteur>& acteur : acteurs.enSpan())
		os << *acteur;
	return os;
}

ostream& operator<< (ostream& os, const Film& film)
{
	return film.output(os);
}

ostream& Livre::output(ostream& os) const
{
	os << "Titre: " << titre << endl;
	os << "  Auteur: " << auteur << "  Année : " << annee << endl;
	os << "  Millions de copies vendues: " << millionsDeCopiesVendus << " M copies" << endl;
	os << "  nombre de pages: " << nombresDePages << " pages" << endl;
	return os;
}

ostream& operator<< (ostream& os, const Livre& livre)
{
	return livre.output(os);
}

ostream& FilmLivre::output(ostream& os) const
{
	os << "Titre: " << Item::titre << endl;
	os << "  Réalisateur: " << realisateur << "  Année : " << Affichable::annee << endl;
	os << "  Recette: " << recette << " M$" << endl;
	os << "  Acteurs:" << endl;
	for (const shared_ptr<Acteur>& acteur : acteurs.enSpan())
		os << *acteur;
	os << endl << "Base sur le livre de l'auteur: " << auteur << endl;
	os << "  Millions de copies vendues: " << millionsDeCopiesVendus << " M copies" << endl;
	os << "  nombre de pages: " << nombresDePages << " pages" << endl;
	return os;
}

ostream& operator<< (ostream& os, const FilmLivre& filmLivre)
{
	return filmLivre.output(os);
}

Item* trouver(const vector<Item> bibliotheque, const auto& critere)
{
	for (auto& item : bibliotheque)
		if (critere(item))
			return item.get();
	return nullptr;
}


int main()
{
#ifdef VERIFICATION_ALLOCATION_INCLUS
	bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
#endif
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	ListeFilms listeFilms = creerListe("films.bin");

	vector<Item> bibliotheque;
	transfererFilms(bibliotheque, listeFilms);
	ajouterLivres(bibliotheque, "livres.txt");

	cout << ligneDeSeparation;

	for (int i : range(size(bibliotheque))) {
		//cout << bibliotheque[i].titre << bibliotheque[i].annee << endl;
		cout << bibliotheque[i] << endl;
	}
	//cout << bibliotheque;
	
	//FilmLivre filmLivre();

	// Détruire tout avant de terminer le programme.
	listeFilms.detruire(true);
}
