#pragma once

#include <string>
#include <cassert>
#include "gsl/span"
#include <memory>
using gsl::span;
using namespace std;

struct Film; struct Acteur; 

class ListeFilms {
public:
	ListeFilms() = default;
	ListeFilms(const std::string& nomFichier);
	ListeFilms(const ListeFilms& l) { assert(l.elements == nullptr); }
	~ListeFilms();
	void ajouterFilm(Film* film);
	void enleverFilm(const Film* film);
	shared_ptr<Acteur> trouverActeur(const string& nomActeur) const;
	span<Film*> enSpan() const;
	int size() const { return nElements; }
	Film*& operator[] (int index) { return elements[index]; }
	


private:
	void changeDimension(int nouvelleCapacite);

	int capacite = 0, nElements = 0;
	Film** elements = nullptr; // Pointeur vers un tableau de Film*, chaque Film* pointant vers un Film.
	bool possedeLesFilms_ = false; // Les films seront détruits avec la liste si elle les possède.
};

struct ListeActeurs {
	int capacite = 0, nElements = 0;
	std::unique_ptr<std::shared_ptr<Acteur>[]> elements;

	ListeActeurs(int capacite = 0, int nElements = 0)
		: capacite(capacite), nElements(nElements)
	{
		elements = make_unique<shared_ptr<Acteur>[]>(capacite);
	}
	ListeActeurs& operator= (const ListeActeurs& autre)
	{
		if (this != &autre)
		{
			capacite = autre.capacite;
			nElements = autre.nElements;

			elements = make_unique<shared_ptr<Acteur>[]>(nElements = autre.capacite);
			for (int i = 0; i < nElements; i++)
			{
				elements[i] = autre.elements[i]; ///////////////////make_shared<Acteur>(autre.elements[i]); 
			}

		}
		return *this;
	}
	ListeActeurs(const Film& autre) : capacite(0), nElements(0), elements(nullptr)
	{
		*this = autre;
	};
};


struct Film
{
	std::string titre, realisateur; 
	int anneeSortie = 0, recette = 0; 
	ListeActeurs acteurs;

	Film() = default;

	Film(const string& titre, const string& realisateur, int anneeSortie, int recette)
		: titre(titre), realisateur(realisateur), anneeSortie(anneeSortie), recette(recette) {}

	Film& operator= (const Film& autre) 
	{
		if (this != &autre) {
			titre = autre.titre;
			realisateur = autre.realisateur;
			anneeSortie = autre.anneeSortie;
			recette = autre.recette;
			acteurs = autre.acteurs;
		}
		return *this;
	};
	Film(const Film& autre) 
	{
		*this = autre;
	};
};

struct Acteur
{
	std::string nom; int anneeNaissance = 0; char sexe = '\0';

	Acteur(const string& nom, int anneeNaissance, char sexe) : nom(nom), anneeNaissance(anneeNaissance), sexe(sexe) {}

	Acteur() = default;

	Acteur& operator= (const Acteur& autre)
	{
		if (this != &autre) {
			nom = autre.nom;
			anneeNaissance = autre.anneeNaissance;
			sexe = autre.sexe;
		}
		return *this;
	};
	Acteur(const Acteur& autre)
	{
		*this = autre;
	};
};
