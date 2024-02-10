﻿#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>

#include "cppitertools/range.hpp"
#include "gsl/span"

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
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
		erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");
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

//TODO: Une fonction pour ajouter un Film à une ListeFilms, le film existant déjà; on veut uniquement ajouter le pointeur vers le film existant.  
// Cette fonction doit doubler la taille du tableau alloué, avec au minimum un élément, dans le cas où la capacité est insuffisante pour ajouter l'élément.  
// Il faut alors allouer un nouveau tableau plus grand, copier ce qu'il y avait dans l'ancien, et éliminer l'ancien trop petit.  
// Cette fonction ne doit copier aucun Film ni Acteur, elle doit copier uniquement des pointeurs.
void ajouterFilm(Film* nouveauFilm, ListeFilms& listeFilms)
{
	if (listeFilms.nElements >= listeFilms.capacite)
	{
		int nouvelleCapacite = 2 * listeFilms.capacite;

		Film** nouvelleListe = new Film * [nouvelleCapacite];

		for (int i = 0; i < listeFilms.nElements; i++)
		{
			nouvelleListe[i] = listeFilms.elements[i];
		}
		delete[] listeFilms.elements;
		listeFilms.capacite = nouvelleCapacite;
		listeFilms.elements = nouvelleListe;
	}
	listeFilms.elements[listeFilms.nElements++] = nouveauFilm;
}

//TODO: Une fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; 
// la fonction prenant en paramètre un pointeur vers le film à enlever. 
//  L'ordre des films dans la liste n'a pas à être conservé.

void enleverFilm(const Film* film, ListeFilms& listeFilms)
{
	for (int i = 0; i < listeFilms.nElements; i++)
	{
		if (listeFilms.elements[i] == film)
		{
			for (int j = i; j < listeFilms.nElements - 1; j++)
			{
				listeFilms.elements[j] = listeFilms.elements[j + 1];
			}
			listeFilms.nElements--;
			return;
		}
	}
}

//TODO: Une fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé. 
//  Devrait utiliser span.

Acteur* trouverActeur(const string& nomActeur, const ListeFilms& listeFilms)
{

	for (auto film : span<Film*>(listeFilms.elements, listeFilms.nElements))
	{
		for (auto acteur : span<Acteur*>(film->acteurs.elements, film->acteurs.nElements))
		{
			if (acteur->nom == nomActeur)
			{
				return acteur;
			}
		}
	}
	return nullptr; // Retourner nullptr si l'acteur n'est pas trouvé
}

//TODO: Compléter les fonctions pour lire le fichier et créer/allouer une ListeFilms.  La ListeFilms devra être passée entre les fonctions, 
// pour vérifier l'existence d'un Acteur avant de l'allouer à nouveau (cherché par nom en utilisant la fonction ci-dessus).
Acteur* lireActeur(istream& fichier, const ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom = lireString(fichier);
	acteur.anneeNaissance = int(lireUintTailleVariable(fichier));
	acteur.sexe = char(lireUintTailleVariable(fichier));

	Acteur* acteurExistant = trouverActeur(acteur.nom, listeFilms); // acteur ou nullptr

	if (acteurExistant != nullptr)
	{
		return acteurExistant; // Retourner un pointeur vers l'acteur existant
	}
	else
	{
		cout << "Création Acteur " << acteur.nom << endl;
		// Allouer un nouvel acteur et retourner un pointeur vers celui-ci
		Acteur* nouvelActeur = new Acteur();
		nouvelActeur->nom = acteur.nom;
		nouvelActeur->anneeNaissance = acteur.anneeNaissance;
		nouvelActeur->sexe = acteur.sexe;
		return nouvelActeur; //TODO: Retourner un pointeur soit vers un acteur existant ou un nouvel acteur ayant les bonnes informations, selon si l'acteur existait déjà.  
		//Pour fins de débogage, affichez les noms des acteurs crées; vous ne devriez pas voir le même nom d'acteur affiché deux fois pour la création.
	}
}

Film* lireFilm(istream& fichier, const ListeFilms& listeFilms)
{
	Film* film = {};
	film->titre = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = int(lireUintTailleVariable(fichier));
	film->recette = int(lireUintTailleVariable(fichier));
	film->acteurs.nElements = int(lireUintTailleVariable(fichier));

	//NOTE: Vous avez le droit d'allouer d'un coup le tableau pour les acteurs, 
	// sans faire de réallocation comme pour ListeFilms.  Vous pouvez aussi copier-coller les fonctions 
	// d'allocation de ListeFilms ci-dessus dans des nouvelles fonctions et faire un remplacement de Film par Acteur, pour réutiliser cette réallocation.

	for (int i = 0; i < film->acteurs.nElements; i++) {
		Acteur* acteur = lireActeur(fichier, listeFilms); //TODO: Placer l'acteur au bon endroit dans les acteurs du film.
		//TODO: Ajouter le film à la liste des films dans lesquels l'acteur joue.
		if (acteur != nullptr)
		{
			ajouterFilm(film, acteur->joueDans);
		}
	}
	return film; //TODO: Retourner le pointeur vers le nouveau film.
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = int(lireUintTailleVariable(fichier));

	ListeFilms listeFilms;
	listeFilms.capacite = nElements;
	listeFilms.nElements = 0;

	for (int i = 0; i < nElements; i++)
	{
		Film* film = lireFilm(fichier, listeFilms); //TODO: Ajouter le film à la liste.
		if (film != nullptr)
		{
			listeFilms.elements[i] = film;
			listeFilms.nElements++;
		}
	}

	return listeFilms; //TODO: Retourner la liste de films.
}

//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film,
//  et les acteurs qui ne jouent plus dans aucun films de la collection).  
// Noter qu'il faut enleve le film détruit des films dans lesquels jouent les acteurs.  
// Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.



void detruire(Film* film, ListeFilms& listeFilms)
{

	enleverFilm(film, listeFilms);


	for (auto& acteur : span<Acteur*>(film->acteurs.elements, film->acteurs.nElements))
	{
		bool joueDansDautresFilms = false;
		for (auto& filmJoue : span<Film*>(acteur->joueDans.elements, acteur->joueDans.nElements))
		{
			if (filmJoue != film)
			{
				joueDansDautresFilms = true;
				break;
			}
		}

		if (!joueDansDautresFilms)
		{
			cout << "Acteur detruit : " << acteur->nom << endl;
			delete acteur;
		}

	}
	delete film;
}

//TODO: Une fonction pour détruire une ListeFilms et tous les films qu'elle contient.
void detruireListeFilms(ListeFilms& listeFilms)
{
	for (auto film : span<Film*>(listeFilms.elements, listeFilms.nElements))
	{
		detruire(film, listeFilms);
	}
	delete[] listeFilms.elements;
	listeFilms.elements = nullptr;
	listeFilms.nElements = 0;
	listeFilms.capacite = 0;
}


void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

//TODO: Une fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
void afficherFilm(const Film& film)
{
	cout << "Titre du film: " << film.titre << endl;
	cout << "Réalisateur: " << film.realisateur << endl;
	cout << "Année de sortie: " << film.anneeSortie << endl;
	cout << "Recette: " << film.recette << endl;
	cout << "Acteurs: " << endl;
	for (auto* acteur : span<Acteur*>(film.acteurs.elements, film.acteurs.nElements))
	{
		afficherActeur(*acteur);
	}

}

//void afficherListeFilms(const ListeFilms& listeFilms)
//{
//	//TODO: Utiliser des caractères Unicode pour définir la ligne de séparation (différente des autres lignes de séparations dans ce progamme).
//	const std::wstring ligneDeSeparation = L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
//	wcout << ligneDeSeparation;
//	//TODO: Changer le for pour utiliser un span.
//	
//	for (Film* film : span<Film*>(listeFilms.elements, listeFilms.nElements))
//	{
//		// Afficher le titre du film
//		cout << "Titre: " << film->titre << endl;
//		// Afficher le réalisateur du film
//		cout << "Réalisateur: " << film->realisateur << endl;
//		// Afficher l'année de sortie du film
//		cout << "Année de sortie: " << film->anneeSortie << endl;
//		// Afficher la recette du film
//		cout << "Recette: " << film->recette << " millions de dollars" << endl;
//		// Afficher les acteurs du film en utilisant la fonction afficherActeur
//		cout << "Acteurs:\n";
//		for (int i = 0; i < film->acteurs.nElements; ++i) {
//			afficherActeur(*(film->acteurs.elements[i]));
//		}
//		//TODO: Afficher le film.
//		wcout << ligneDeSeparation;
//	}
//}

//void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
//{
//	Acteur* acteur = trouverActeur(nomActeur, listeFilms);
//	if (acteur == nullptr)
//		cout << "Aucun acteur de ce nom" << endl;
//	else
//		afficherListeFilms(acteur->joueDans);
//}


//int main()
//{
//	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.
//
//	//int* fuite = new int; //TODO: Enlever cette ligne après avoir vérifié qu'il y a bien un "Fuite detectee" de "4 octets" affiché à la fin de l'exécution, qui réfère à cette ligne du programme.
//
//	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";
//
//	//TODO: Chaque TODO dans cette fonction devrait se faire en 1 ou 2 lignes, en appelant les fonctions écrites.
//
//	//TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.  Devrait afficher les noms de 20 acteurs sans doublons (par l'affichage pour fins de débogage dans votre fonction lireActeur).
//	ListeFilms listeFilms = creerListe("films.bin");
//
//	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
//	//TODO: Afficher le premier film de la liste.  Devrait être Alien.
//
//	cout << ligneDeSeparation << "Les films sont:" << endl;
//	//TODO: Afficher la liste des films.  Il devrait y en avoir 7.
//
//	//TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.
//
//	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
//	//TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
//
//	//TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
//
//	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
//	//TODO: Afficher la liste des films.
//
//	//TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new" et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.
//
//	//TODO: Détruire tout avant de terminer le programme.  La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee." a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque des delete.
//}
int main()
{
	bibliotheque_cours::activerCouleursAnsi();
	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	// Lire le fichier binaire en créant la liste de films
	ListeFilms listeFilms = creerListe("films.bin");

	// Afficher le premier film de la liste
	/*cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	cout << listeFilms.elements[0]->titre << endl;*/

	//	// Afficher la liste des films
	//	cout << ligneDeSeparation << "Les films sont:" << endl;
	//	afficherListeFilms(listeFilms);
	//
	//	// Modifier l'année de naissance de Benedict Cumberbatch
	//	Acteur* cumberbatch = trouverActeur("Benedict Cumberbatch",  listeFilms);
	//	if (cumberbatch != nullptr) {
	//		cumberbatch->anneeNaissance = 1976;
	//	}
	//
	//	// Afficher les films dans lesquels Benedict Cumberbatch joue
	//	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	//	afficherFilmographieActeur(listeFilms, "Benedict Cumberbatch");
	//
	//	// Détruire et enlever le premier film de la liste (Alien)
	//	detruire(listeFilms.elements[0], listeFilms);
	//
	//	// Afficher la liste des films après la suppression
	//	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	//	afficherListeFilms(listeFilms);
	//
	//	// Détruire tout avant de terminer le programme
	//	//detruireListeFilms(listeFilms);
	//
	//	// Appels supplémentaires pour avoir 0% de lignes non exécutées
	//	// Il peut s'agir de déclarations de nouvelles fonctions ou de lignes de code supplémentaires selon les besoins
	//
	return 0;
}
